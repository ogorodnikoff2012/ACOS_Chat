#include <server/listener.h>
#include <logger.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <poll.h>
#include <ts_vector/ts_vector.h>
#include <sys/ioctl.h>

input_msg_event_t *new_input_msg_event(connection_t *conn) {
    input_msg_event_t *evt = calloc(1, sizeof(input_msg_event_t));
    evt->e_hdr.type = INPUT_MSG_EVENT_TYPE;
    evt->conn = conn;
    return evt;
}

static void connection_deleter(void *ptr) {
    connection_t *conn = (connection_t *) ptr;
    close(conn->sockid);
    if (conn->header != NULL) {
        free(conn->header);
    }
    if (conn->body != NULL) {
        free(conn->body);
    }
    free(conn);
}

static char *prompt(const char *str) {
    printf("%s: ", str);
    fflush(stdout);
    char *s = NULL;
    size_t n = 0;
    int len = getline(&s, &n, stdin);
    if (len < 0) {
        free(s);
        return NULL;
    }
    return s;
}

connection_t *new_connection(int sockid) {
    connection_t *conn = calloc(1, sizeof(connection_t));
    conn->sockid = sockid;
    conn->bytes_read = 0;
    conn->bytes_expected = MSG_HEADER_SIZE;
    conn->body = NULL;
    conn->header = NULL;
    return conn;
}

bool listener_init(listener_data_t *data) {
    ts_map_init(&data->clients);
    event_loop_init(&data->event_loop);
    char *port_str = prompt("Port (leave blank for default 1337)"); 
    int port = 1337;
    if (port_str != NULL) {
       int p = atoi(port_str);
       if (p > 0) {
           port = p;
       }
       free(port_str);
    }

    data->server_socket.sockid = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (data->server_socket.sockid == -1) {
        LOG("ERROR: Cannot open socket");
        return false;
    }

    struct sockaddr_in sock_port;
    sock_port.sin_family = AF_INET;
    sock_port.sin_port = htons(port);
    sock_port.sin_addr.s_addr = INADDR_ANY;
    int status = bind(data->server_socket.sockid, 
            (struct sockaddr *) &sock_port, sizeof(sock_port));
    if (status == -1) {
        LOG("ERROR: Cannot bind socket");
        return false;
    }

    status = listen(data->server_socket.sockid, 1);
    if (status == -1) {
        return false;
    }
    LOG("Server is listening at port %d", port);
    return true;
}

void listener_destroy(listener_data_t *data) {
    close(data->server_socket.sockid);
    LOG("Server is closed");
    ts_map_destroy(&data->clients, connection_deleter);
    event_loop_destroy(&data->event_loop);
}

typedef struct {
    void *first, *second;
} pair_t;

static void push_back_conn(uint64_t key, void *val, void *env) {
    ts_vector_t *v = (ts_vector_t *) env;
    connection_t *conn = (connection_t *) val;
    struct pollfd fd;
    fd.fd = conn->sockid;
    fd.events = POLLIN;
    fd.revents = 0;
    if (!conn->in_worker) {
        ts_vector_push_back(v, &fd);
    }
}

static void close_connection(listener_data_t *data, int sockid) {
    connection_t *conn = ts_map_erase(&data->clients, sockid);
    connection_deleter(conn);
    LOG("Connection closed, socket %d", sockid);
}

static void process_connection(listener_data_t *data, int sockid) {
    LOG("Incoming data, socket %d", sockid);
    connection_t *conn = ts_map_find(&data->clients, sockid);
    int count;
    if (conn->bytes_read < 5) {
        if (conn->header == NULL) {
            conn->header = malloc(MSG_HEADER_SIZE);
            if (conn->header == NULL) {
                LOG("ERROR while malloc(), closing connection");
                close_connection(data, sockid);
                return;
            }
        }
        count = recv(sockid, conn->header + conn->bytes_read, conn->bytes_expected - conn->bytes_read, 0);
    } else {
        if (conn->body == NULL) {
            int body_size = ntohl(*(int *)(conn->header + 1));
            conn->body = malloc(body_size);
            if (conn->body == NULL) {
                LOG("ERROR while malloc(), closing connection");
                close_connection(data, sockid);
                return;
            }
            conn->bytes_expected += body_size;
        }
        count = recv(sockid, conn->body + conn->bytes_read - MSG_HEADER_SIZE,
                     conn->bytes_expected - conn->bytes_read, 0);
    }
    if (count <= 0) {
        LOG("%s while recv(), closing connection", (count == -1 ? "ERROR" : "EOF"));
        close_connection(data, sockid);
        return;
    }
    conn->bytes_read += count;
    if (conn->bytes_expected == conn->bytes_read && conn->bytes_expected > MSG_HEADER_SIZE) {
        LOG("Recieved message, processing...");
        conn->in_worker = true;
        send_event(data->controller_event_loop, (event_t *) new_input_msg_event(conn));
    }
}

static void accept_connection(listener_data_t *data) {
    LOG("Incoming connection");
    struct sockaddr client_addr;
    unsigned int addr_len = sizeof(client_addr);
    int sockid = accept(data->server_socket.sockid, &client_addr, &addr_len);
    if (sockid > 0) {
        connection_t *conn = new_connection(sockid);
        ts_map_insert(&data->clients, sockid, conn);
        LOG("Accepted, socket %d", sockid);
    } else {
        LOG("Failed");
    }
}

void *listener_thread(void *ptr) {
    listener_data_t *data = (listener_data_t *) ptr;
    bool work = true;

    ts_vector_t polling_conns;
    ts_vector_init(&polling_conns, sizeof(struct pollfd));

    struct pollfd server_socket;

    while (work) {
        ts_vector_clear(&polling_conns);
        
        server_socket.fd = data->server_socket.sockid;
        server_socket.events = POLLIN;
        server_socket.revents = 0;

        ts_vector_push_back(&polling_conns, &server_socket);

        ts_map_forall(&data->clients, &polling_conns, push_back_conn);

        int res = poll(polling_conns.data, polling_conns.size, 100); // 100ms
        if (res == -1) {
            LOG("poll() failed");
            work = false;
            continue;
        }

        if (res > 0) {
            struct pollfd *arr = polling_conns.data;

            if (arr[0].revents & POLLIN) {
                accept_connection(data);
            }

            for (int i = 1; i < polling_conns.size; ++i) {
                int revents = arr[i].revents;
                if (revents & (POLLHUP | POLLERR)) {
                    close_connection(data, arr[i].fd);
                } else if (revents & POLLIN) {
                    process_connection(data, arr[i].fd);
                }   
            }
        }

        work = event_loop_iteration(&data->event_loop, data);
    }
    ts_vector_destroy(&polling_conns);
    pthread_exit(NULL);
}

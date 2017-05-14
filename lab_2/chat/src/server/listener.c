#include <server/listener.h>
#include <common/logger.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <poll.h>
#include <ts_vector/ts_vector.h>
#include <common/connection.h>
#include <server/events/input_msg_event.h>
#include <server/events/close_connection_event.h>
#include <server/misc.h>
#include <server/db.h>
#include <server/events/broadcast_message_event.h>

bool listener_init(listener_data_t *data) {
    ts_map_init(&data->clients);
    event_loop_init(&data->event_loop);
    ts_map_insert(&data->event_loop.handlers, CLOSE_CONNECTION_EVENT_TYPE, close_connection_event_handler);
    data->conn_mgr = NULL;
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

void close_connection(listener_data_t *data, int sockid) {
    int uid = conn_mgr_get_uid(&data->controller->conn_mgr, sockid);
    int sessions = conn_mgr_get_number_of_sessions(&data->controller->conn_mgr, uid);

    connection_t *conn = ts_map_erase(&data->clients, sockid);
    conn_mgr_del_sid(data->conn_mgr, sockid);
    connection_deleter(conn);
    LOG("Connection closed, socket %d", sockid);

    if (sessions == 1) {
        char *msg = NULL;
        char *login = get_login_by_uid(data->controller->db, uid);
        asprintf(&msg, "User '%s' has logged out", login);
        free(login);
        send_event(data->controller_event_loop,
                   (event_t *) new_broadcast_message_event(get_tstamp(), MESSAGE_SERVER_META, NULL,
                                                           msg, NULL_UID));
    }
}

static void process_connection(listener_data_t *data, int sockid) {
#define BAD_COUNT   LOG("%s while recv(), closing connection", (count == -1 ? "ERROR" : "EOF")); \
                    close_connection(data, sockid); \
                    return;
#define BAD_MALLOC  LOG("ERROR while malloc(), closing connection"); \
                    close_connection(data, sockid); \
                    return;

    LOG("Incoming data, socket %d", sockid);
    connection_t *conn = ts_map_find(&data->clients, sockid);
    int count = 0;
    if (conn->bytes_read < MSG_HEADER_SIZE) {
        if (conn->header == NULL) {
            conn->header = malloc(MSG_HEADER_SIZE);
            if (conn->header == NULL) {
                BAD_MALLOC;
            }
        }
        count = recv(sockid, conn->header + conn->bytes_read, conn->bytes_expected - conn->bytes_read, 0);
        if (count <= 0) {
            BAD_COUNT;
        }
    };

    conn->bytes_read += count;
    if (conn->bytes_read < MSG_HEADER_SIZE) {
        return;
    }

    int body_size = ntohl(*(int *)(conn->header + 1));
    if (body_size < 0) {
        LOG("Message is corrupted");
        close_connection(data, sockid);
        return;
    }
    if (body_size > 0) {
        if (conn->body == NULL) {
            conn->body = malloc(body_size);
            if (conn->body == NULL) {
                BAD_MALLOC;
            }
            conn->bytes_expected += body_size;
        }
        count = recv(sockid, conn->body + conn->bytes_read - MSG_HEADER_SIZE,
                     conn->bytes_expected - conn->bytes_read, 0);
        if (count <= 0) {
            BAD_COUNT;
        }

        conn->bytes_read += count;
    }

#undef BAD_MALLOC
#undef BAD_COUNT
    if (conn->bytes_read == conn->bytes_expected) {
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
        conn_mgr_add_sid(data->conn_mgr, sockid);
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

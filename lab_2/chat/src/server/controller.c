#include <server/controller.h>
#include <server/worker.h>
#include <server/listener.h>
#include <stdlib.h>
#include <logger.h>
#include <arpa/inet.h>
#include <defines.h>
#include <pascal_string.h>
#include <ts_vector/ts_vector.h>
#include <unistd.h>
#include <server/conn_mgr.h>

static void broadcast_message_event_deleter(event_t *ptr) {
    broadcast_message_event_t *evt = (broadcast_message_event_t *) ptr;
    free(evt->login);
    free(evt->msg);
    free(evt);
}

typedef struct {
    void *first, *second;
} pair_t;

static void broadcast_message_callback(uint64_t key, void *val, void *env) {
    controller_data_t *data = ((pair_t *) env)->second;
    broadcast_message_event_t *evt = ((pair_t *) env)->first;
    send_event(&data->workers_event_loop,
               (event_t *) new_send_message_job(key, evt->tstamp, evt->type,
                                                evt->login == NULL ? NULL : strdup(evt->login), strdup(evt->msg)));
}

static void broadcast_message_event_handler(event_t *ptr, void *dptr) {
    broadcast_message_event_t *evt = (broadcast_message_event_t *) ptr;
    controller_data_t *data = dptr;

    if (evt->type == MESSAGE_SERVER_REGULAR) {
        int rc;
        const char *tail;
        sqlite3_stmt *stmt;
        sqlite3_prepare(data->db, "INSERT INTO messages VALUES(?001, ?002, ?003);", -1, &stmt, &tail);
        sqlite3_bind_int(stmt, 1, evt->uid);
        sqlite3_bind_text(stmt, 2, evt->msg, -1, SQLITE_STATIC);
        sqlite3_bind_int64(stmt, 3, evt->tstamp);

        while ((rc = sqlite3_step(stmt)) != SQLITE_DONE) {
            if (rc == SQLITE_ERROR) {
                break;
            }
        }
        sqlite3_finalize(stmt);
    }

    pair_t pair;
    pair.first = evt;
    pair.second = data;
    ts_map_forall(&data->conn_mgr.sid_to_uid, &pair, broadcast_message_callback);
}

broadcast_message_event_t *new_broadcast_message_event(uint64_t tstamp, char type, char *login, char *msg, int uid) {
    broadcast_message_event_t *evt = calloc(1, sizeof(broadcast_message_event_t));
    evt->e_hdr.type = BROADCAST_MESSAGE_EVENT_TYPE;
    evt->tstamp = tstamp;
    evt->msg = msg;
    evt->login = login;
    evt->uid = uid;
    evt->type = type;

    return evt;
}

void send_status_code(int sockid, int status) {
    char buffer[MSG_HEADER_SIZE + 2 * sizeof(uint32_t)];
    buffer[0] = 's';
    *(int *)(buffer + 1) = htonl(2 * sizeof(uint32_t));
    *(int *)(buffer + 5) = htonl(sizeof(uint32_t)); /* Yes, I know that magic constants are awful  */
    *(int *)(buffer + 9) = htonl(status);

    int stat = send(sockid, buffer, MSG_HEADER_SIZE + 2 * sizeof(uint32_t), 0);
    LOG("Sent status code, result = %d", stat);
}

static void decrease_workers(controller_data_t *data) {
    send_urgent_event(&data->workers_event_loop, &EXIT_EVT);
}

static void kill_worker(worker_data_t *data) {
    pthread_join(data->thread, NULL);
    --data->controller->workers_cnt;
}

static pascal_string_t *pstrdup(pascal_string_t *str) {
    uint32_t len = sizeof(uint32_t) + str->length;
    pascal_string_t *dup = malloc(len);
    memcpy(dup, str, len);
    return dup;
}

static void input_msg_event_handler(event_t *ptr, void *dptr) {
    input_msg_event_t *evt = (input_msg_event_t *) ptr;
    controller_data_t *data = (controller_data_t *) dptr;
    char *header = evt->conn->header;
    char *body = evt->conn->body;

    int msglen = ntohl(*(int *)(header + 1));
    LOG("Found message: type \'%c\', length %d", header[0], msglen);
    
    ts_vector_t *tokens = calloc(1, sizeof(ts_vector_t));
    ts_vector_init(tokens, sizeof(message_token_t));

    char *iter = body, *end = body + msglen;

    while (iter < end) {
        pascal_string_t *str = (pascal_string_t *) iter;
        str->length = ntohl(str->length);
        char *next = iter + 4 + str->length;
        if (next > end) {
            LOG("Message corrupted");
            evt->conn->in_worker = false;
            send_status_code(evt->conn->sockid, MSG_STATUS_INVALID_MSG);
            for (int i = 0; i < tokens->size; ++i) {
                free(((message_token_t *) tokens->data)[i].data.p_str);
            }
            ts_vector_destroy(tokens);
            free(tokens);
            return;
        }
        message_token_t token;
        token.type = DATA_P_STR;
        token.data.p_str = pstrdup(str);
        ts_vector_push_back(tokens, &token);
        iter = next;
    }

    LOG("End of message");
    send_event(&data->workers_event_loop,
               (event_t *) new_process_message_job(evt->conn,
                                                   new_server_message(header[0], tokens)));
}

static void input_msg_event_deleter(event_t *ptr) {
    input_msg_event_t *evt = (input_msg_event_t *) ptr;
    free(evt->conn->header);
    free(evt->conn->body);
    evt->conn->bytes_read = 0;
    evt->conn->bytes_expected = MSG_HEADER_SIZE;
    evt->conn->header = NULL;
    evt->conn->body = NULL;
    free(evt);
}

static uint64_t unique_id() {
    static uint64_t id = 0;
    return ++id;
}

static void run_worker(controller_data_t *data) {
    worker_data_t *worker = calloc(1, sizeof(worker_data_t));
    worker->controller = data;
    worker->event_loop = &data->workers_event_loop;
    worker->controller_event_loop = &data->event_loop;
    worker->worker_id = unique_id();
    pthread_create(&worker->thread, NULL, worker_thread, worker);
    ts_map_insert(&data->workers, worker->worker_id, worker);
    ++data->workers_cnt;
}

static void worker_exit_event_handler(event_t *ptr, void *dptr) {
    worker_exit_event_t *evt = (worker_exit_event_t *) ptr;
    controller_data_t *data = (controller_data_t *) dptr;

    worker_data_t *worker = ts_map_erase(&data->workers, evt->worker_id);
    if (worker != NULL) {
        kill_worker(worker);
        free(worker);
    }
}

worker_exit_event_t *new_worker_exit_event(int worker_id) {
    worker_exit_event_t *evt = calloc(1, sizeof(worker_exit_event_t));
    evt->e_hdr.type = WORKER_EXIT_EVENT_TYPE;
    evt->worker_id = worker_id;
    return evt;
}

static void change_worker_cnt_event_handler(event_t *ptr, void *dptr) {
    change_worker_cnt_event_t *evt = (change_worker_cnt_event_t *) ptr;
    controller_data_t *data = (controller_data_t *) dptr;
    int delta = evt->cnt - data->workers_cnt;
    while (delta > 0) {
        run_worker(data);
        --delta;
    }
    while (delta < 0) {
        decrease_workers(data);
        ++delta;
    }
}

change_worker_cnt_event_t *new_change_worker_cnt_event(int cnt) {
    change_worker_cnt_event_t *evt = calloc(1, sizeof(change_worker_cnt_event_t));
    evt->e_hdr.type = CHANGE_WORKER_CNT_EVENT_TYPE;
    evt->cnt = cnt;
    return evt;
}

int controller_init(controller_data_t *data) {
    data->workers_cnt = 0;
    ts_map_init(&data->workers);
    event_loop_init(&data->event_loop);
    event_loop_init(&data->workers_event_loop);
    workers_init(&data->workers_event_loop);

    ts_map_insert(&data->event_loop.handlers, CHANGE_WORKER_CNT_EVENT_TYPE, change_worker_cnt_event_handler);
    ts_map_insert(&data->event_loop.handlers, WORKER_EXIT_EVENT_TYPE, worker_exit_event_handler);

    ts_map_insert(&data->event_loop.handlers, INPUT_MSG_EVENT_TYPE, input_msg_event_handler);
    ts_map_insert(&data->event_loop.deleters, INPUT_MSG_EVENT_TYPE, input_msg_event_deleter);

    ts_map_insert(&data->event_loop.handlers, BROADCAST_MESSAGE_EVENT_TYPE, broadcast_message_event_handler);
    ts_map_insert(&data->event_loop.deleters, BROADCAST_MESSAGE_EVENT_TYPE, broadcast_message_event_deleter);

    init_conn_mgr(&data->conn_mgr);
    int rc = sqlite3_open(DB_FILENAME, &data->db);
    return rc;
}

void controller_destroy(controller_data_t *data) {
    destroy_conn_mgr(&data->conn_mgr);
    ts_map_destroy(&data->workers, (void (*)(void *)) kill_worker);
    if (data->db != NULL) {
        int rc;
        while ((rc = sqlite3_close(data->db)) != SQLITE_OK) {
            usleep(100000); /* 100 ms */
        }
    }
    event_loop_destroy(&data->workers_event_loop);
    event_loop_destroy(&data->event_loop);
}

void *controller_thread(void *ptr) {
    LOG("Starting controller...");
    controller_data_t *data = (controller_data_t *) ptr;
    run_event_loop(&data->event_loop, data);

    LOG("Stopping workers...");
    for (int i = 0; i < data->workers_cnt; ++i) {
        decrease_workers(data);
    }
    
    ts_map_erase(&data->event_loop.handlers, CHANGE_WORKER_CNT_EVENT_TYPE);
    ts_map_erase(&data->event_loop.handlers, INPUT_MSG_EVENT_TYPE);
    while (data->workers_cnt > 0) {
        event_loop_iteration(&data->event_loop, data);
        usleep(10000); /* 10 ms */
    }
    
    LOG("Stopping controller...");
    pthread_exit(NULL);
}

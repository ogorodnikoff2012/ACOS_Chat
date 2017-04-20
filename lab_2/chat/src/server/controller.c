#include <server/controller.h>
#include <server/worker.h>
#include <server/listener.h>
#include <stdlib.h>
#include <logger.h>
#include <arpa/inet.h>
#include <defines.h>
#include <pascal_string.h>
#include <ts_vector/ts_vector.h>

static void decrease_workers(controller_data_t *data) {
    send_urgent_event(&data->workers_event_loop, &EXIT_EVT);
}

static void kill_worker(worker_data_t *data) {
    pthread_join(data->thread, NULL);
    --data->controller->workers_cnt;
}

static void input_msg_event_handler(event_t *ptr, void *dptr) {
    input_msg_event_t *evt = (input_msg_event_t *) ptr;
    controller_data_t *data = (controller_data_t *) dptr;
    char *header = evt->conn->header;
    char *body = evt->conn->body;

    int msglen = ntohl(*(int *)(header + 1));
    LOG("Found message: type \'%c\', length %d", header[0], msglen);
    
    ts_vector_t strings;
    ts_vector_init(&strings, sizeof(char *));

    char *iter = body, *end = body + msglen;

    while (iter < end) {
        pascal_string_t *str = (pascal_string_t *) iter;
        str->length = ntohl(str->length);
        char *next = iter + 4 + str->length;
        if (next > end) {
            LOG("Message corrupted");
            /* send event about it */
            return;
        }
        
        char *c_str = strndup(str->data, str->length);
        LOG("Found string: %s", c_str);
        ts_vector_push_back(&strings, &c_str);
        iter = next;
    }

    LOG("End of message");
    for (int i = 0; i < strings.size; ++i) {
        free(*(char **)(strings.data + i));
    }
    ts_vector_destroy(&strings);
}

static void input_msg_event_deleter(event_t *ptr) {
    input_msg_event_t *evt = (input_msg_event_t *) ptr;
    free(evt->conn->header);
    free(evt->conn->body);
    evt->conn->bytes_read = 0;
    evt->conn->bytes_expected = MSG_HEADER_SIZE;
    evt->conn->header = NULL;
    evt->conn->body = NULL;
    evt->conn->in_worker = false;
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
    worker->db = data->db;
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

change_worker_cnt_event_t *new_change_worker_cnt_event_t(int cnt) {
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

    ts_map_insert(&data->event_loop.handlers, CHANGE_WORKER_CNT_EVENT_TYPE, change_worker_cnt_event_handler);
    ts_map_insert(&data->event_loop.handlers, WORKER_EXIT_EVENT_TYPE, worker_exit_event_handler);

    ts_map_insert(&data->event_loop.handlers, INPUT_MSG_EVENT_TYPE, input_msg_event_handler);
    ts_map_insert(&data->event_loop.deleters, INPUT_MSG_EVENT_TYPE, input_msg_event_deleter);


    int rc = sqlite3_open(DB_FILENAME, &data->db);
    return rc;
}

void controller_destroy(controller_data_t *data) {
    for (int i = 0; i < data->workers_cnt; ++i) {
        decrease_workers(data);
    }
    if (data->db != NULL) {
        sqlite3_close(data->db);
    }
    ts_map_destroy(&data->workers, (void (*)(void *)) kill_worker);
    event_loop_destroy(&data->workers_event_loop);
    event_loop_destroy(&data->event_loop);
}

void *controller_thread(void *ptr) {
    LOG("Starting controller...");
    controller_data_t *data = (controller_data_t *) ptr;
    run_event_loop(&data->event_loop, data);
    LOG("Stopping controller...");
    pthread_exit(NULL);
}

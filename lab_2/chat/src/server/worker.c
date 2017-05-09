#define _GNU_SOURCE
#include <server/worker.h>
#include <server/controller.h>
#include <pthread.h>
#include <stdio.h>
#include <logger.h>
#include <stdlib.h>
#include <ts_vector/ts_vector.h>
#include <netinet/in.h>
#include <server/listener.h>
#include <xenon_md5/xenon_md5.h>
#include <unistd.h>
#include <sys/time.h>
#include <pascal_string.h>

static uint64_t pack_tstamp(uint64_t n) {
    return (((uint64_t) htonl((uint32_t) (n >> 32))) << 32) | htonl((uint32_t) n);
}

static void send_message_job_handler(event_t *ptr, void *dptr) {
    send_message_job_t *job = (send_message_job_t *) ptr;
    worker_data_t *data = dptr;

    ts_vector_t *tokens = calloc(1, sizeof(ts_vector_t));
    ts_vector_init(tokens, sizeof(message_token_t));

    message_token_t buf_token;

    buf_token.type = DATA_INT64;
    buf_token.data.i64 = pack_tstamp(job->tstamp);

    ts_vector_push_back(tokens, &buf_token);
    if (job->type == MESSAGE_SERVER_REGULAR) {
        buf_token.type = DATA_C_STR;
        buf_token.data.c_str = job->login;
        ts_vector_push_back(tokens, &buf_token);
    }
    buf_token.type = DATA_C_STR;
    buf_token.data.c_str = job->msg;
    ts_vector_push_back(tokens, &buf_token);

    server_message_t *msg = new_server_message(job->type, tokens);
    void *package = pack_message(msg);
    delete_server_message(msg);

    int pack_len = ntohl(*(int *)(package + 1)) + MSG_HEADER_SIZE;
    int count = send(job->sid, package, pack_len, 0);
    if (count == -1) {
        LOG("Message sending failed");
    }
    free(package);
}

send_message_job_t *new_send_message_job(int sid, uint64_t tstamp, char type, char *login, char *msg) {
    send_message_job_t *job = calloc(1, sizeof(send_message_job_t));
    job->e_hdr.type = SEND_MESSAGE_JOB_TYPE;
    job->sid = sid;
    job->tstamp = tstamp;
    job->login = login;
    job->msg = msg;
    job->type = type;

    return job;
}

void delete_server_message(server_message_t *msg) {
    for (int i = 0; i < msg->tokens->size; ++i) {
        message_token_t token = ((message_token_t *) (msg->tokens->data))[i];
        switch (token.type) {
            case DATA_C_STR:
                free(token.data.c_str);
                break;
            case DATA_INT64:
                break;
            case DATA_INT32:
                break;
            case DATA_P_STR:
                free(token.data.p_str);
                break;
            default:
                break;
        }
    }
    ts_vector_destroy(msg->tokens);
    free(msg->tokens);
    free(msg);
}

void *pack_message(server_message_t *msg) {
    uint32_t size = MSG_HEADER_SIZE;
    for (int i = 0; i < msg->tokens->size; ++i) {
        size += 4;
        message_token_t token = ((message_token_t *) (msg->tokens->data))[i];
        switch (token.type) {
            case DATA_C_STR:
                size += strlen(token.data.c_str);
                break;
            case DATA_INT64:
                size += sizeof(uint64_t);
                break;
            case DATA_INT32:
                size += sizeof(uint32_t);
                break;
            case DATA_P_STR:
                size += token.data.p_str->length;
                break;
            default:
                LOG("Corrupted message");
                break;
        }
    }
    void *buffer = malloc(size);
    void *it = buffer;

    *(char *)it = msg->msg_type;
    it += sizeof(char);
    *(uint32_t *)it = htonl(size - MSG_HEADER_SIZE);
    it += sizeof(uint32_t);

    for (int i = 0; i < msg->tokens->size; ++i) {
        uint32_t *len_ptr = (uint32_t *) it;
        it += sizeof(uint32_t);

        message_token_t token = ((message_token_t *) (msg->tokens->data))[i];
        switch (token.type) {
            case DATA_C_STR: {
                *len_ptr = strlen(token.data.c_str);
                memcpy(it, token.data.c_str, *len_ptr);
            }
                break;
            case DATA_INT64:
                *len_ptr = sizeof(uint64_t);
                *((uint64_t *) it) = token.data.i64;
                break;
            case DATA_INT32:
                *len_ptr = sizeof(uint32_t);
                *((uint32_t *) it) = token.data.i32;
                break;
            case DATA_P_STR:
                *len_ptr = token.data.p_str->length;
                memcpy(it, token.data.p_str->data, *len_ptr);
                break;
            default:
                *len_ptr = 0;
                break;
        }
        it += *len_ptr;
        *len_ptr = htonl(*len_ptr);
    }
    return buffer;
}

static void process_message_job_deleter(event_t *ptr) {
    process_message_job_t *job = (process_message_job_t *) ptr;
    delete_server_message(job->msg);
    job->conn->in_worker = false;
    free(job);
}

static int login_or_register(sqlite3 *db, char *login, char *passwd) {
    char *hash = md5hex(passwd);
    int rc;
    const char *tail = NULL;
    sqlite3_stmt *stmt;

    sqlite3_prepare(db, "SELECT uid, password_hash FROM users WHERE login = ?001;",
                    -1, &stmt, &tail);
    sqlite3_bind_text(stmt, 1, login, -1, SQLITE_STATIC);

    int rows = 0;
    int uid = -MSG_STATUS_AUTH_ERROR;

    while ((rc = sqlite3_step(stmt)) != SQLITE_DONE) {
        while (rc == SQLITE_BUSY) {
            usleep(100); /* 100 us */
            rc = sqlite3_step(stmt);
        }
        if (rc != SQLITE_ROW) {
            sqlite3_finalize(stmt);
            free(hash);
            return -MSG_STATUS_AUTH_ERROR;
        }
        ++rows;
        if (strcmp(hash, sqlite3_column_text(stmt, 1)) == 0) {
            uid = sqlite3_column_int(stmt, 0);
        }
    }
    sqlite3_finalize(stmt);

    if (rows == 0) {
        sqlite3_prepare(db, "SELECT MAX(uid) + 1 FROM USERS;", -1, &stmt, &tail);
        while ((rc = sqlite3_step(stmt)) != SQLITE_DONE) {
            while (rc == SQLITE_BUSY) {
                usleep(100); /* 100 us */
                rc = sqlite3_step(stmt);
            }
            if (rc != SQLITE_ROW) {
                sqlite3_finalize(stmt);
                free(hash);
                return -MSG_STATUS_REG_ERROR;
            }
            uid = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);

        sqlite3_prepare(db, "INSERT INTO users VALUES (?001, ?002, ?003);", -1, &stmt, &tail);
        sqlite3_bind_int(stmt, 1, uid);
        sqlite3_bind_text(stmt, 2, login, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, hash, -1, SQLITE_STATIC);

        while ((rc = sqlite3_step(stmt)) != SQLITE_DONE) {
            if (rc == SQLITE_ERROR) {
                sqlite3_finalize(stmt);
                return -MSG_STATUS_REG_ERROR;
            }
        }
        sqlite3_finalize(stmt);
    }

    free(hash);
    return uid;
}

static char *get_login_by_uid(sqlite3 *db, int uid) {
    sqlite3_stmt *stmt;
    const char *tail;
    sqlite3_prepare(db, "SELECT login FROM users WHERE uid = ?001;", -1, &stmt, &tail);
    sqlite3_bind_int(stmt, 1, uid);

    int rc;
    char *ans = NULL;
    while ((rc = sqlite3_step(stmt)) != SQLITE_DONE) {
        while (rc == SQLITE_BUSY) {
            usleep(100); /* 100 us */
            rc = sqlite3_step(stmt);
        }
        if (rc != SQLITE_ROW) {
            sqlite3_finalize(stmt);
            return NULL;
        }
        if (ans != NULL) {
            free(ans);
        }
        ans = strdup(sqlite3_column_text(stmt, 0));
    }
    sqlite3_finalize(stmt);
    return ans;
}

static uint64_t get_tstamp() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t tstamp = tv.tv_sec;
    tstamp <<= 32;
    return tstamp | (uint32_t) (tv.tv_usec);
}

static char *p_str_to_c_str(pascal_string_t *p_str) {
    return strndup(p_str->data, p_str->length);
}

static void process_message_job_handler(event_t *ptr, void *dptr) {
    process_message_job_t *job = (process_message_job_t *) ptr;
    worker_data_t *data = (worker_data_t *) dptr;
    LOG("Worker #%d is processing a message...", data->worker_id);
    message_token_t *tokens = job->msg->tokens->data;
    switch (job->msg->msg_type) {
        case MESSAGE_CLIENT_LOGIN: {
            if (job->msg->tokens->size < 2) {
                send_status_code(job->conn->sockid, MSG_STATUS_INVALID_MSG);
                break;
            }
            char *login  = p_str_to_c_str(tokens[0].data.p_str);
            char *passwd = p_str_to_c_str(tokens[1].data.p_str);
            int uid = login_or_register(data->controller->db, login, passwd);
            if (uid < 0) {
                send_status_code(job->conn->sockid, -uid);
                break;
            }
            int sessions = conn_mgr_get_number_of_sessions(&data->controller->conn_mgr, uid);
            conn_mgr_add_uid(&data->controller->conn_mgr, uid);
            conn_mgr_connect_sid_uid(&data->controller->conn_mgr, job->conn->sockid, uid);
            send_status_code(job->conn->sockid, MSG_STATUS_OK);
            LOG("User '%s' has logged in, uid = %d", login, uid);
            if (sessions == 0) {
                char *msg = NULL;
                asprintf(&msg, "User '%s' has logged in", login);
                send_event(data->controller_event_loop,
                           (event_t *) new_broadcast_message_event(get_tstamp(), MESSAGE_SERVER_META, NULL,
                                                                   msg, NULL_UID));
            }
        }
            break;
        case MESSAGE_CLIENT_LOGOUT: {
            int uid = conn_mgr_get_uid(&data->controller->conn_mgr, job->conn->sockid);
            int sessions = conn_mgr_get_number_of_sessions(&data->controller->conn_mgr, uid);
            send_event(data->controller->listener_event_loop,
                       (event_t *) new_close_connection_event(job->conn->sockid));
            if (sessions == 1) {
                char *msg = NULL;
                asprintf(&msg, "User '%s' has logged out", get_login_by_uid(data->controller->db, uid));
                send_event(data->controller_event_loop,
                           (event_t *) new_broadcast_message_event(get_tstamp(), MESSAGE_SERVER_META, NULL,
                                                                   msg, NULL_UID));
            }
        }
            break;
        case MESSAGE_CLIENT_REGULAR: {
            int uid = conn_mgr_get_uid(&data->controller->conn_mgr, job->conn->sockid);
            if (uid == NULL_UID) {
                send_status_code(job->conn->sockid, MSG_STATUS_UNAUTHORIZED);
                break;
            }
            if (job->msg->tokens->size < 1) {
                send_status_code(job->conn->sockid, MSG_STATUS_INVALID_MSG);
                break;
            }
            char *login = get_login_by_uid(data->controller->db, uid);
            char *msg = p_str_to_c_str(tokens[0].data.p_str);
            send_event(data->controller_event_loop,
                       (event_t *) new_broadcast_message_event(get_tstamp(), MESSAGE_SERVER_REGULAR, login, msg, uid));
        }
            break;
        default:
            send_status_code(job->conn->sockid, MSG_STATUS_INVALID_TYPE);
            break;
    }
}

server_message_t *new_server_message(char msg_type, ts_vector_t *tokens) {
    server_message_t *msg = calloc(1, sizeof(server_message_t));
    msg->msg_type = msg_type;
    msg->tokens = tokens;
    return msg;
}

process_message_job_t *new_process_message_job(connection_t *conn, server_message_t *msg) {
    process_message_job_t *job = calloc(1, sizeof(process_message_job_t));
    job->e_hdr.type = PROCESS_MESSAGE_JOB_TYPE;
    job->msg = msg;
    job->conn = conn;
    return job;
}

void workers_init(event_loop_t *event_loop) {
    ts_map_insert(&event_loop->handlers, PROCESS_MESSAGE_JOB_TYPE, process_message_job_handler);
    ts_map_insert(&event_loop->deleters, PROCESS_MESSAGE_JOB_TYPE, process_message_job_deleter);

    ts_map_insert(&event_loop->handlers, SEND_MESSAGE_JOB_TYPE, send_message_job_handler);
}

void *worker_thread(void *ptr) {
    worker_data_t *data = (worker_data_t *) ptr;
    LOG("Worker #%d starting...", data->worker_id);
    run_event_loop(data->event_loop, data);
    LOG("Worker #%d stopping...", data->worker_id);
    send_event(data->controller_event_loop, (event_t *) new_worker_exit_event(data->worker_id));
    pthread_exit(NULL);
}

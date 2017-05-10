//
// Created by xenon on 09.05.17.
//

#include <server/events/process_message_job.h>
#include <server/worker.h>
#include <logger.h>
#include <server/misc.h>
#include <server/events/broadcast_message_event.h>
#include <server/db.h>
#include <server/events/close_connection_event.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <pair.h>
#include <server/controller.h>
#include <server/connection.h>
#include <server/conn_mgr.h>
#include <ts_vector/ts_vector.h>
#include <server/server_message.h>
#include <pascal_string.h>

process_message_job_t *new_process_message_job(connection_t *conn, server_message_t *msg) {
    process_message_job_t *job = calloc(1, sizeof(process_message_job_t));
    job->e_hdr.type = PROCESS_MESSAGE_JOB_TYPE;
    job->msg = msg;
    job->conn = conn;
    return job;
}


void process_message_job_deleter(event_t *ptr) {
    process_message_job_t *job = (process_message_job_t *) ptr;
    delete_server_message(job->msg);
    job->conn->in_worker = false;
    free(job);
}

static void browse_history_callback(void *env, sqlite3_stmt *stmt) {
    pair_t *p = env;
    sqlite3 *db = p->first;
    int sid = *(int *) p->second;

    int uid = sqlite3_column_int(stmt, 0);
    char *msg = strdup((const char *) sqlite3_column_text(stmt, 1));
    uint64_t tstamp = (uint64_t) sqlite3_column_int64(stmt, 2);
    char *login = get_login_by_uid(db, uid);

    ts_vector_t *tokens = calloc(1, sizeof(ts_vector_t));
    ts_vector_init(tokens, sizeof(message_token_t));

    message_token_t buf_token;

    buf_token.type = DATA_INT64;
    buf_token.data.i64 = tstamp;
    ts_vector_push_back(tokens, &buf_token);

    buf_token.type = DATA_C_STR;
    buf_token.data.c_str = login;
    ts_vector_push_back(tokens, &buf_token);

    buf_token.type = DATA_C_STR;
    buf_token.data.c_str = msg;
    ts_vector_push_back(tokens, &buf_token);

    server_message_t *s_msg = new_server_message(MESSAGE_SERVER_HISTORY, tokens);
    void *package = pack_message(s_msg);
    delete_server_message(s_msg);

    int pack_len = ntohl(*(int *)(package + 1)) + MSG_HEADER_SIZE;
    int count = send(sid, package, pack_len, 0);
    if (count == -1) {
        LOG("Message sending failed");
    }
    free(package);
}

static void user_list_callback(uint64_t key, void *val, void *ptr) {
    ts_vector_t *uids = ptr;
    int uid = (int) key;
    if (ts_map_size(val) != 0 && uid != NULL_UID) {
        ts_vector_push_back(uids, &uid);
    }
}

static void kick_callback(uint64_t key, void *value, void *ptr) {
    int sockid = (int) key;
    pair_t *pair = ptr;
    event_loop_t *eloop = pair->first;
    pascal_string_t *reason = pair->second;

    message_token_t token;
    token.type = DATA_P_STR;
    token.data.p_str = pstrdup(reason);

    ts_vector_t *tokens = calloc(1, sizeof(ts_vector_t));
    ts_vector_init(tokens, sizeof(message_token_t));
    ts_vector_push_back(tokens, &token);

    server_message_t *msg = new_server_message(MESSAGE_SERVER_KICK, tokens);

    void *package = pack_message(msg);
    delete_server_message(msg);

    int pack_len = ntohl(*(int *)(package + 1)) + MSG_HEADER_SIZE;
    int count = send(sockid, package, pack_len, 0);
    if (count == -1) {
        LOG("Message sending failed");
    }
    free(package);

    send_event(eloop, (event_t *) new_close_connection_event(sockid));
}

void process_message_job_handler(event_t *ptr, void *dptr) {
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
                free(login);
                free(passwd);
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
            free(login);
            free(passwd);
        }
            break;
        case MESSAGE_CLIENT_LOGOUT: {
            int uid = conn_mgr_get_uid(&data->controller->conn_mgr, job->conn->sockid);
            int sessions = conn_mgr_get_number_of_sessions(&data->controller->conn_mgr, uid);
            send_event(data->controller->listener_event_loop,
                       (event_t *) new_close_connection_event(job->conn->sockid));
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
        case MESSAGE_CLIENT_HISTORY: {
            if (job->msg->tokens->size < 1 || tokens[0].data.p_str->length != 4) {
                send_status_code(job->conn->sockid, MSG_STATUS_INVALID_MSG);
                break;
            }
            int cnt = ntohl(*(int *)(tokens[0].data.p_str->data));
            pair_t env;
            env.first = data->controller->db;
            env.second = &job->conn->sockid;
            browse_history(data->controller->db, cnt, &env, browse_history_callback);
        }
            break;
        case MESSAGE_CLIENT_LIST: {
            ts_vector_t *users = calloc(1, sizeof(ts_vector_t));
            ts_vector_init(users, sizeof(int));
            ts_map_forall(&data->controller->conn_mgr.uid_to_sid, users, user_list_callback);

            ts_vector_t *tokens = calloc(1, sizeof(ts_vector_t));
            ts_vector_init(tokens, sizeof(message_token_t));

            message_token_t token;
            uint32_t *uids = (uint32_t *) users->data;
            for (int i = 0; i < users->size; ++i) {
                token.type = DATA_INT32;
                token.data.i32 = uids[i];
                ts_vector_push_back(tokens, &token);

                token.type = DATA_C_STR;
                token.data.c_str = get_login_by_uid(data->controller->db, uids[i]);
                ts_vector_push_back(tokens, &token);
            }

            ts_vector_destroy(users);
            free(users);
            server_message_t *msg = new_server_message(MESSAGE_SERVER_LIST, tokens);

            void *package = pack_message(msg);
            delete_server_message(msg);

            int pack_len = ntohl(*(int *)(package + 1)) + MSG_HEADER_SIZE;
            int count = send(job->conn->sockid, package, pack_len, 0);
            if (count == -1) {
                LOG("Message sending failed");
            }
            free(package);
        }
            break;
        case MESSAGE_CLIENT_KICK: {
            int uid = conn_mgr_get_uid(&data->controller->conn_mgr, job->conn->sockid);
            if (uid != ROOT_UID) {
                send_status_code(job->conn->sockid, MSG_STATUS_ACCESS_ERROR);
                break;
            }
            if (job->msg->tokens->size < 2 || tokens[0].data.p_str->length != 4) {
                send_status_code(job->conn->sockid, MSG_STATUS_INVALID_MSG);
                break;
            }
            int kuid = ntohl(*(int *)(tokens[0].data.p_str->data));
            pascal_string_t *reason = tokens[1].data.p_str;

            pair_t pair;
            pair.first = (void *) data->controller->listener_event_loop;
            pair.second = reason;
            conn_mgr_forall_uid(&data->controller->conn_mgr, kuid, &pair, kick_callback);

            char *msg = NULL;
            char *login = get_login_by_uid(data->controller->db, kuid);
            asprintf(&msg, "User '%s' was kicked, reason: %.*s", login, reason->length, reason->data);
            free(login);
            send_event(data->controller_event_loop,
                       (event_t *) new_broadcast_message_event(get_tstamp(), MESSAGE_SERVER_META, NULL,
                                                               msg, NULL_UID));

        }
            break;
        default:
            send_status_code(job->conn->sockid, MSG_STATUS_INVALID_TYPE);
            break;
    }
}

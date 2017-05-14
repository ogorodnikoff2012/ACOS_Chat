//
// Created by xenon on 14.05.17.
//

#include <client/listener.h>
#include <poll.h>
#include <client/controller.h>
#include <client/events/display_parsed_message_job.h>
#include <client/misc.h>
#include <string.h>
#include <client/defines.h>
#include <common/connection.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <common/message.h>
#include <ts_vector/ts_vector.h>
#include <common/pascal_string.h>
#include <client/events/hangup_event.h>
#include <client/events/kick_event.h>
#include <client/events/list_event.h>
#include <client/events/status_event.h>

void client_listener_init(client_listener_data_t *data) {
    event_loop_init(&data->event_loop);
    data->conn.sockid = -1;
    data->conn.body = NULL;
    data->conn.bytes_expected = MSG_HEADER_SIZE;
    data->conn.bytes_read = 0;
    data->conn.header = NULL;
    data->conn.in_worker = false;
}

void client_listener_destroy(client_listener_data_t *data) {
    event_loop_destroy(&data->event_loop);
}

static void process_connection(client_listener_data_t *data, int sockid) {
    int count = 0;
    if (data->conn.bytes_read < MSG_HEADER_SIZE) {
        if (data->conn.header == NULL) {
            data->conn.header = malloc(MSG_HEADER_SIZE);
        }
        count = recv(sockid, data->conn.header + data->conn.bytes_read,
                     data->conn.bytes_expected - data->conn.bytes_read, 0);
        if (count <= 0) {
            return;
        }
    };

    data->conn.bytes_read += count;
    if (data->conn.bytes_read < MSG_HEADER_SIZE) {
        return;
    }

    int body_size = ntohl(*(int *)(data->conn.header + 1));
    if (body_size < 0) {
        return;
    }
    if (body_size > 0) {
        if (data->conn.body == NULL) {
            data->conn.body = malloc(body_size);
            data->conn.bytes_expected += body_size;
        }
        count = recv(sockid, data->conn.body + data->conn.bytes_read - MSG_HEADER_SIZE,
                     data->conn.bytes_expected - data->conn.bytes_read, 0);
        if (count <= 0) {
            return;
        }

        data->conn.bytes_read += count;
    }

    if (data->conn.bytes_read == data->conn.bytes_expected) {
        message_t *msg = unpack_message_2(data->conn.header, data->conn.body);
        message_token_t *tokens = msg->tokens->data;
        switch (msg->msg_type) {
            case MESSAGE_SERVER_REGULAR:
            case MESSAGE_SERVER_HISTORY:
            case MESSAGE_SERVER_META: {
                int iter = 0;
                pascal_string_t *tstamp = tokens[iter++].data.p_str;
                pascal_string_t *login = (msg->msg_type == MESSAGE_SERVER_META) ? NULL : tokens[iter++].data.p_str;
                pascal_string_t *text = tokens[iter++].data.p_str;

                parsed_message_t *pmsg = new_parsed_message(msg->msg_type, p_str_to_c_str(login),
                                                            p_str_to_c_str(text), be64toh(*(uint64_t *) tstamp->data));
                delete_message(msg);
                send_event(data->controller->gui_event_loop, (event_t *) new_display_parsed_message_job(pmsg));
            }
                break;
            case MESSAGE_SERVER_KICK:
                send_event(&data->controller->event_loop,
                           (event_t *) new_kick_event(p_str_to_c_str(tokens[0].data.p_str)));
                delete_message(msg);
                break;
            case MESSAGE_SERVER_LIST:
                send_event(&data->controller->event_loop, (event_t *) new_list_event(msg));
                break;
            case MESSAGE_SERVER_STATUS:
                send_event(&data->controller->event_loop,
                           (event_t *) new_status_event(htonl(* (uint32_t *) tokens[0].data.p_str->data)));
                delete_message(msg);
                break;
            default:
                delete_message(msg);
                break;
        }

        free(data->conn.body);
        free(data->conn.header);

        data->conn.body = NULL;
        data->conn.bytes_expected = MSG_HEADER_SIZE;
        data->conn.bytes_read = 0;
        data->conn.header = NULL;
    }
}

void *client_listener_thread(void *ptr) {
    client_listener_data_t *data = ptr;
    bool work = true;

    struct pollfd fds[1];
    while (work) {
        if (data->controller->sockid != -1) {
            fds[0].fd = data->controller->sockid;
            fds[0].events = POLLIN;
            fds[0].revents = 0;

            int res = poll(fds, 1, 100);
            if (res < 0) {
                send_event(&data->controller->event_loop, (event_t *) new_hangup_event());
                send_event(data->controller->gui_event_loop, (event_t *) new_display_parsed_message_job(
                        new_parsed_message(MESSAGE_CLIENT_INTERNAL, NULL,
                                           strdup("Cannot poll(), please restart client"), get_tstamp())));
                work = false;
                continue;
            }

            if (res > 0) {
                if (fds[0].revents & (POLLHUP | POLLERR)) {
                    send_event(&data->controller->event_loop, (event_t *) new_hangup_event());
                    send_event(data->controller->gui_event_loop, (event_t *) new_display_parsed_message_job(
                            new_parsed_message(MESSAGE_CLIENT_INTERNAL, NULL,
                                               strdup("Connection lost, please restart client"), get_tstamp())));
                    work = false;
                    continue;
                } else if (fds[0].revents & POLLIN) {
                    process_connection(data, fds[0].fd);
                }
            }
        }
        work = event_loop_iteration(&data->event_loop, data);
    }
    pthread_exit(NULL);
}

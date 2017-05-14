//
// Created by xenon on 13.05.17.
//

#include <client/controller.h>
#include <client/defines.h>
#include <client/events/send_message_to_server_job.h>
#include <sys/socket.h>
#include <unistd.h>
#include <client/events/readinput_event.h>
#include <string.h>
#include <stdlib.h>
#include <client/events/display_parsed_message_job.h>
#include <client/misc.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pair.h>
#include <common/message.h>
#include <client/events/list_event.h>
#include <client/events/kick_event.h>
#include <client/events/status_event.h>
#include <client/events/hangup_event.h>

void client_controller_init(client_controller_data_t *data) {
    event_loop_init(&data->event_loop);
    ts_map_insert(&data->event_loop.handlers, SEND_MESSAGE_TO_SERVER_JOB_TYPE, send_message_to_server_job_handler);
    ts_map_insert(&data->event_loop.deleters, SEND_MESSAGE_TO_SERVER_JOB_TYPE, send_message_to_server_job_deleter);

    ts_map_insert(&data->event_loop.handlers, LIST_EVENT_TYPE, list_event_handler);
    ts_map_insert(&data->event_loop.deleters, LIST_EVENT_TYPE, list_event_deleter);
    ts_map_insert(&data->event_loop.handlers, KICK_EVENT_TYPE, kick_event_handler);
    ts_map_insert(&data->event_loop.deleters, KICK_EVENT_TYPE, kick_event_deleter);
    ts_map_insert(&data->event_loop.handlers, HANGUP_EVENT_TYPE, hangup_event_handler);
    ts_map_insert(&data->event_loop.deleters, HANGUP_EVENT_TYPE, hangup_event_deleter);
    ts_map_insert(&data->event_loop.handlers, STATUS_EVENT_TYPE, status_event_handler);

    data->sockid = -1;
    data->logged_in = false;
}

void client_controller_destroy(client_controller_data_t *data) {
    /* Send 'logout' message */
    if (data->sockid != -1) {
        char buf[5];
        buf[0] = MESSAGE_CLIENT_LOGOUT;
        buf[1] = buf[2] = buf[3] = buf[4] = 0;
        send(data->sockid, buf, 5, 0);
        close(data->sockid);
    }
    event_loop_destroy(&data->event_loop);
}

static bool connect_to_server(char *name, int port, client_controller_data_t *data) {
    int sockid = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockid < 0) {
        return false;
    }

    struct hostent *h = gethostbyname(name);
    if (h == NULL) {
        return false;
    }

    in_addr_t **addr_list = (in_addr_t **) h->h_addr_list;
    struct sockaddr_in addrport;

    addrport.sin_family = AF_INET;
    addrport.sin_port = htons(port);
    addrport.sin_addr.s_addr = *addr_list[0];

    int res = connect(sockid, (struct sockaddr *) &addrport, sizeof(addrport));
    if (res == 0) {
        data->sockid = sockid;
    }
    return res == 0;
}

static void get_server_addr_callback_failure(void *ptr) {
    client_controller_data_t *data = ptr;
    send_event(data->gui_event_loop, (event_t *) new_display_parsed_message_job(new_parsed_message(
            MESSAGE_CLIENT_INTERNAL, NULL, strdup("Cannot connect to server, please restart client"), get_tstamp()
    )));
    send_urgent_event(&data->event_loop, &EXIT_EVT);
}

void login_callback_failure(void *ptr) {
    pair_t *p = ptr;
    client_controller_data_t *data = p->first;
    send_event(data->gui_event_loop, (event_t *) new_display_parsed_message_job(new_parsed_message(
            MESSAGE_CLIENT_INTERNAL, NULL, strdup("Cannot login, please restart client"), get_tstamp()
    )));
    send_urgent_event(&data->event_loop, &EXIT_EVT);
    if (p->second != NULL) {
        free(p->second);
    }
    free(p);
}

static void login_callback_success(char *password, void *ptr) {
    pair_t *p = ptr;
    client_controller_data_t *data = p->first;
    char *login = p->second;

    ts_vector_t *tokens = calloc(1, sizeof(ts_vector_t));
    ts_vector_init(tokens, sizeof(message_token_t));
    message_token_t token;

    token.type = DATA_C_STR;
    token.data.c_str = login;
    ts_vector_push_back(tokens, &token);

    token.type = DATA_C_STR;
    token.data.c_str = password;
    ts_vector_push_back(tokens, &token);

    message_t *msg = new_message(MESSAGE_CLIENT_LOGIN, tokens);
    send_message(msg, data->sockid);
    delete_message(msg);

    free(p);
}

static void get_username_callback_success(char *str, void *ptr) {
    pair_t *p = ptr;
    client_controller_data_t *data = p->first;
    p->second = str;
    send_event(data->gui_event_loop, (event_t *) new_readinput_event(
            strdup("Enter password:"), true, p, login_callback_success, login_callback_failure)
    );
}

static void get_server_addr_callback_success(char *str, void *ptr) {
    client_controller_data_t *data = ptr;
    char *port_pos = strchr(str, ':');
    int port = 1337;
    if (port_pos != NULL) {
        port = atoi(port_pos + 1);
        *port_pos = '\0';
    }

    if (!connect_to_server(str, port, data)) {
        get_server_addr_callback_failure(data);
        free(str);
        return;
    }

    free(str);
    pair_t *p = calloc(1, sizeof(pair_t));
    p->first = data;
    p->second = NULL;
    send_event(data->gui_event_loop, (event_t *) new_readinput_event(
            strdup("Enter username:"), false, p, get_username_callback_success, login_callback_failure)
    );

}

void *client_controller_thread(void *ptr) {
    client_controller_data_t *data = ptr;

    send_event(data->gui_event_loop, (event_t *) new_readinput_event(
            strdup("Enter server address (specify port if it's not 1337):"),
            false, data, get_server_addr_callback_success, get_server_addr_callback_failure)
    );

    run_event_loop(&data->event_loop, data);
    pthread_exit(NULL);
}

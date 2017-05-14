//
// Created by xenon on 13.05.17.
//

#ifndef XENON_CHAT_CLIENT_CONTROLLER_H
#define XENON_CHAT_CLIENT_CONTROLLER_H

#include "../event_loop/event_loop.h"

typedef struct {
    event_loop_t event_loop;
    int sockid;
    bool logged_in;
    event_loop_t *gui_event_loop;
} client_controller_data_t;

void client_controller_init(client_controller_data_t *data);
void client_controller_destroy(client_controller_data_t *data);

void *client_controller_thread(void *ptr);

void login_callback_failure(void *ptr);

#endif // XENON_CHAT_CLIENT_CONTROLLER_H

//
// Created by xenon on 14.05.17.
//

#ifndef XENON_CHAT_CLIENT_LISTENER_H
#define XENON_CHAT_CLIENT_LISTENER_H

#include "../event_loop/event_loop.h"
#include "controller.h"
#include "../common/connection.h"

typedef struct {
    event_loop_t event_loop;
    client_controller_data_t *controller;
    connection_t conn;
} client_listener_data_t;

void client_listener_init(client_listener_data_t *data);
void client_listener_destroy(client_listener_data_t *data);

void *client_listener_thread(void *ptr);

#endif // XENON_CHAT_CLIENT_LISTENER_H

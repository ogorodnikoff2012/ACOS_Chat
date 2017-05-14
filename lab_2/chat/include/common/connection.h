//
// Created by xenon on 09.05.17.
//

#ifndef XENON_CHAT_SERVER_CONNECTION_H
#define XENON_CHAT_SERVER_CONNECTION_H

#include <stdbool.h>

typedef struct {
    int sockid;
    void *header, *body;
    int bytes_read, bytes_expected;
    bool in_worker;
} connection_t;

void connection_deleter(void *ptr);
connection_t *new_connection(int sockid);

#endif // XENON_CHAT_SERVER_CONNECTION_H

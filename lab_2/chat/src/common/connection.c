//
// Created by xenon on 09.05.17.
//

#include <common/connection.h>
#include <server/defines.h>
#include <unistd.h>
#include <stdlib.h>

void connection_deleter(void *ptr) {
    connection_t *conn = (connection_t *) ptr;
    close(conn->sockid);
    if (conn->header != NULL) {
        free(conn->header);
    }
    if (conn->body != NULL) {
        free(conn->body);
    }
    free(conn);
}

connection_t *new_connection(int sockid) {
    connection_t *conn = calloc(1, sizeof(connection_t));
    conn->sockid = sockid;
    conn->bytes_read = 0;
    conn->bytes_expected = MSG_HEADER_SIZE;
    conn->body = NULL;
    conn->header = NULL;
    conn->in_worker = false;
    return conn;
}


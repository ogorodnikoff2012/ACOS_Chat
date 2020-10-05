//
// Created by xenon on 13.05.17.
//

#include <client/parsed_message.h>
#include <stdlib.h>

parsed_message_t *new_parsed_message(char type, char *author, char *text, uint64_t tstamp) {
    parsed_message_t *msg = calloc(1, sizeof(parsed_message_t));
    msg->type = type;
    msg->author = author;
    msg->text = text;
    msg->tstamp = tstamp;
    return msg;
}

void delete_parsed_message(parsed_message_t *msg) {
    if (msg->author != NULL) {
        free(msg->author);
    }
    if (msg->text != NULL) {
        free(msg->text);
    }
    free(msg);
}

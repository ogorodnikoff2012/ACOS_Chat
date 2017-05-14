//
// Created by xenon on 13.05.17.
//

#ifndef CHAT_PARSED_MESSAGE_H
#define CHAT_PARSED_MESSAGE_H

#include <stdint.h>

typedef struct {
    char type;
    char *author, *text;
    uint64_t tstamp;
} parsed_message_t;

parsed_message_t *new_parsed_message(char type, char *author, char *text, uint64_t tstamp);
void delete_parsed_message(parsed_message_t *msg);

#endif //CHAT_PARSED_MESSAGE_H

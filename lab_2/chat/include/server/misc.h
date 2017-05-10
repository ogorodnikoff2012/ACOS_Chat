//
// Created by xenon on 09.05.17.
//

#ifndef XENON_CHAT_SERVER_MISC_H
#define XENON_CHAT_SERVER_MISC_H

#include <stdint.h>

uint64_t get_tstamp();
void send_status_code(int sockid, int status);
uint64_t unique_id();
char *prompt(const char *str);
int to_range(int min, int val, int max);

#endif // XENON_CHAT_SERVER_MISC_H

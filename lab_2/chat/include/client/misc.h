//
// Created by xenon on 13.05.17.
//

#ifndef XENON_CHAT_CLIENT_MISC_H
#define XENON_CHAT_CLIENT_MISC_H

#include <sys/time.h>
#include <stdint.h>
#include "controller.h"

uint64_t get_tstamp();
struct timeval parse_tstamp(uint64_t tstamp);
char *str_strip(const char *str);
const char *status_str(int status);
void ask_for_history(client_controller_data_t *data);
void ask_for_userlist(client_controller_data_t *data);

#endif // XENON_CHAT_CLIENT_MISC_H

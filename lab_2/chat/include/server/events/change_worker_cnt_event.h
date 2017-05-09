//
// Created by xenon on 09.05.17.
//

#ifndef XENON_CHAT_SERVER_EVENTS_CHANGE_WORKER_CNT_EVENT_H
#define XENON_CHAT_SERVER_EVENTS_CHANGE_WORKER_CNT_EVENT_H

#include "../../event_loop/event_loop.h"
#include "../defines.h"

typedef struct {
    event_t e_hdr;
    int cnt;
} change_worker_cnt_event_t;

change_worker_cnt_event_t *new_change_worker_cnt_event(int cnt);
void change_worker_cnt_event_handler(event_t *ptr, void *dptr);


#endif // XENON_CHAT_SERVER_EVENTS_CHANGE_WORKER_CNT_EVENT_H

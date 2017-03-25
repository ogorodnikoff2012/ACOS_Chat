#define _GNU_SOURCE
#include <event_loop.h>
#include <stdlib.h>

/* Static functions */

static void default_deleter(event_t *evt) {
    free(evt);
}

static void event_destructor(void *evt) {
    event_t *e = (event_t *) evt;
    e->deleter(e);
}

/* Global functions */

event_t EXIT_EVT = {0};

void event_loop_init(event_loop_t *el) {
    ts_queue_init(&el->event_queue);
    ts_map_init(&el->handlers);
    ts_map_init(&el->deleters);
}

void event_loop_destroy(event_loop_t *el) {
    ts_queue_destroy(&el->event_queue, event_destructor);
    ts_map_destroy(&el->deleters, NULL);
    ts_map_destroy(&el->handlers, NULL);
}

void run_event_loop(event_loop_t *el) {
    bool work = true;
    while (work) {
        while (!ts_queue_empty(&el->event_queue)) {
            event_t *evt = ts_queue_pop(&el->event_queue);
            if (evt == NULL) {
                continue;
            }
            if (evt->type == EXIT_EVENT_TYPE) {
                work = false;
                break;
            }
            evt_handler_t handler = ts_map_find(&el->handlers, evt->type);
            if (handler != NULL) {
                handler(evt);
            }
            evt->deleter(evt);
        }
        pthread_yield();
    }
}

void send_event(event_loop_t *el, event_t *evt) {
    evt->deleter = ts_map_find(&el->deleters, evt->type);
    if (evt->deleter == NULL) {
        evt->deleter = default_deleter;
    }
    ts_queue_push(&el->event_queue, evt);
}

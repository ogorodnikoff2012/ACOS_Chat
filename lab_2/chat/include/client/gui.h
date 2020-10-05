#ifndef XENON_CHAT_CLIENT_GUI_H
#define XENON_CHAT_CLIENT_GUI_H

#include "event_loop/ts_queue/ts_queue.h"
#include "event_loop/event_loop.h"
#include <ncurses.h>
#include <form.h>
#include <sys/time.h>
#include "client/defines.h"
#include "common/message.h"
#include "parsed_message.h"
#include "controller.h"

typedef struct {
    event_loop_t event_loop;
    WINDOW *messages, *messages_pad,
           *input, *input_inner,
           *statusbar;
    size_t scr_width, scr_height,
           messages_height;
    int line;
    FORM *input_form;
    FIELD *input_field[2];
    event_loop_t *controller_event_loop;
    client_controller_data_t *controller;
    bool redraw_messages;
} gui_data_t;

void gui_init(gui_data_t *data);
void gui_destroy(gui_data_t *data);

void *gui_thread(void *);

void gui_print_message(gui_data_t *data, parsed_message_t *msg);

#endif /* XENON_CHAT_CLIENT_GUI_H */

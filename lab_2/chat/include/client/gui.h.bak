#ifndef XENON_CHAT_CLIENT_GUI_H
#define XENON_CHAT_CLIENT_GUI_H

#include "../event_loop/ts_queue/ts_queue.h"
#include "../event_loop/event_loop.h"
#include <ncurses6/ncursesw/ncurses.h>
#include <ncurses6/ncursesw/form.h>

typedef struct {
    ts_queue_t unprinted_messages;
    WINDOW *messages, *messages_inner,
           *input, *input_inner,
           *statusbar;
    size_t scr_width, scr_height;
    FORM *input_form;
    FIELD *input_field[2];
    event_loop_t *controller_event_loop;
} gui_data_t;

void gui_init(gui_data_t *data);
void gui_destroy(gui_data_t *data);

void *gui_thread(void *);

#endif /* XENON_CHAT_CLIENT_GUI_H */

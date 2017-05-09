#ifndef XENON_CHAT_CLIENT_GUI_H
#define XENON_CHAT_CLIENT_GUI_H

#include "../event_loop/ts_queue/ts_queue.h"
#include "../event_loop/event_loop.h"
#include <ncursesw/ncurses.h>
#include <ncursesw/form.h>
#include <sys/time.h>
#include "server/defines.h"

#define MESSAGE_EVENT_TYPE (((GUI_THREAD_ID) << 8) + 1)
#define READINPUT_EVENT_TYPE (((GUI_THREAD_ID) << 8) + 2)

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
    bool redraw_messages;
} gui_data_t;

typedef enum {
    EMT_REGULAR,
    EMT_SERVER,
    EMT_INTERNAL
} msg_type_t;

typedef struct {
    char *author, *msg;
    struct timeval tv;
    msg_type_t type;
} message_t;

typedef struct {
    event_t e_hdr;
    message_t *msg;
} message_event_t;

typedef struct {
    event_t e_hdr;
    char *prompt;
    bool passwd;
    void (*callback_success)(char *, void *);
    void (*callback_error)(void *);
} readinput_event_t;

message_t *new_regular_message(char *author, char *msg, struct timeval tv);
message_t *new_internal_message(char *msg, struct timeval tv);
message_t *new_server_message(char *msg, struct timeval tv);
message_t *new_message(msg_type_t type, char *author, char *msg, struct timeval tv);
void free_message(message_t *msg);

message_event_t *new_message_event(message_t *msg);
readinput_event_t *new_readinput_event(char *prompt, bool passwd,
        void (*callback_success)(char *, void *), 
        void (*callback_error)(void *));

void gui_init(gui_data_t *data);
void gui_destroy(gui_data_t *data);

void *gui_thread(void *);

#endif /* XENON_CHAT_CLIENT_GUI_H */

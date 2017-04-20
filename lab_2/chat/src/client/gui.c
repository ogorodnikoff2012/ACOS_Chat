#include <client/gui.h>
#include <locale.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define KEY_ALTERNATE_BACKSPACE 127

#define INPUT_WINDOW_HEIGHT 10

#define MESSAGE_PAD_WIDTH 120
#define MESSAGE_PAD_HEIGHT 2000

static int max(int a, int b) {
    return a < b ? b : a;
}

static int min(int a, int b) {
    return a < b ? a : b;
}

static void gui_init_windows(gui_data_t *data) {
    getmaxyx(stdscr, data->scr_height, data->scr_width);

    data->statusbar = newwin(1, data->scr_width, data->scr_height - 1, 0);

    data->input = newwin(INPUT_WINDOW_HEIGHT, data->scr_width,
            getbegy(data->statusbar) - INPUT_WINDOW_HEIGHT, 0);
    box(data->input, 0, 0);
    data->input_inner = derwin(data->input, getmaxy(data->input) - 2,
            getmaxx(data->input) - 2, 1, 1);

    data->messages = newwin(getbegy(data->input), data->scr_width, 0, 0);
    box(data->messages, 0, 0);

    data->messages_height = getmaxy(data->messages) - 2;
    data->line = MESSAGE_PAD_HEIGHT - data->messages_height;

    keypad(data->input_inner, TRUE);
    nodelay(data->input_inner, TRUE);
    idlok(data->messages_pad, TRUE);
    scrollok(data->messages_pad, TRUE);

    data->input_field[0] = new_field(getmaxy(data->input_inner),
            getmaxx(data->input_inner), 0, 0, 0, 0);
    data->input_field[1] = NULL;
    
    set_field_back(data->input_field[0], A_UNDERLINE);
    field_opts_off(data->input_field[0], O_AUTOSKIP);

    data->input_form = new_form(data->input_field);

    set_form_win(data->input_form, data->input);
    set_form_sub(data->input_form, data->input_inner);

    post_form(data->input_form);

    wprintw(data->statusbar,
            "Xenon chat v0.1 @ Press F1 to exit, Shift+Enter to send message");

    wrefresh(data->messages);
    prefresh(data->messages_pad, data->line, 0,
            getbegy(data->messages) + 1, getbegx(data->messages) + 1,
            getmaxy(data->messages) - 2, getmaxx(data->messages) - 1);
    wrefresh(data->input);
    wrefresh(data->input_inner);
    wrefresh(data->statusbar);

    wmove(data->input_inner, 0, 0); 
    data->redraw_messages = true;
}

static void gui_kill_windows(gui_data_t *data) {
    unpost_form(data->input_form);
    free_form(data->input_form);
    free_field(data->input_field[0]);

    delwin(data->messages);
    delwin(data->input);
    delwin(data->input_inner);
    delwin(data->statusbar);
}

static void gui_restart_windows(gui_data_t *data) {
    gui_kill_windows(data);
    gui_init_windows(data);
}

static void gui_print_message(gui_data_t *data, message_t *msg) {
    wattr_on(data->messages_pad, A_BOLD, NULL);
    const char *author;
    switch (msg->type) {
        case EMT_REGULAR:
            author = msg->author;
            break;
        case EMT_SERVER:
            author = "<SERVER>";
            break;
        case EMT_INTERNAL:
            author = "<INTERNAL>";
            break;
    }
    wprintw(data->messages_pad, "%s, %s", author, ctime(&msg->tv.tv_sec));
    wattr_off(data->messages_pad, A_BOLD, NULL);
    wprintw(data->messages_pad, "%s\n", msg->msg);
}

static void message_event_handler(event_t *ptr, void *dptr) {
    message_event_t *evt = (message_event_t *) ptr;
    gui_data_t *data = (gui_data_t *) dptr;
    gui_print_message(data, evt->msg);
    data->redraw_messages = true;
}

static void message_event_deleter(event_t *ptr) {
    message_event_t *evt = (message_event_t *) ptr;
    free_message(evt->msg);
    free(evt);
}

message_event_t *new_message_event(message_t *msg) {
    message_event_t *evt = calloc(1, sizeof(message_event_t));
    evt->e_hdr.type = MESSAGE_EVENT_TYPE;
    evt->msg = msg;
    return evt;
}

static char *str_strip(const char *str) {
    char *buf = calloc(strlen(str) + 1, sizeof(char));
    char *buf_it = buf;
    char prev = '\n';
    for (const char *it = str; *it != 0; ++it) {
        bool copy = true;
        if (*it == prev && prev <= ' ') {
            copy = false;
        }
        if (copy) {
            *buf_it++ = *it;
        }
        prev = *it;
    }
    return buf;
}

static void readinput_event_handler(event_t *ptr, void *dptr) {
    readinput_event_t *evt = (readinput_event_t *) ptr;
    gui_data_t *data = (gui_data_t *) dptr;
    
    /* Setting up dialog window */

    FIELD *field[2];
    FORM *form;
    WINDOW *dialog;

    int dialog_height = 7;
    int dialog_width = 60;
    int dialog_y = (data->scr_height - dialog_height) >> 1;
    int dialog_x = (data->scr_width - dialog_width) >> 1;

    int field_height = 1;
    int field_width = 50;
    int field_y = 4;
    int field_x = (dialog_width - field_width) >> 1;

    int prompt_y = 2;
    int prompt_x = field_x;    

    field[0] = new_field(field_height, field_width, field_y, field_x, 0, 0);
    field[1] = NULL;

    set_field_back(field[0], A_UNDERLINE);
    field_opts_off(field[0], O_AUTOSKIP);
    if (evt->passwd) {
        field_opts_off(field[0], O_PUBLIC);
    }
    form = new_form(field);

    dialog = newwin(dialog_height, dialog_width, dialog_y, dialog_x);
    keypad(dialog, TRUE);

    set_form_win(form, dialog);
    set_form_sub(form, derwin(dialog, dialog_height - 2, 
                dialog_width - 2, 1, 1));

    box(dialog, 0, 0);
    post_form(form);

    mvwprintw(dialog, prompt_y, prompt_x, "%s", evt->prompt);
    wrefresh(dialog);

    /* keyboard handling */

    int ch;
    while ((ch = wgetch(dialog)) != '\n' && ch != KEY_F(1)) {
        switch (ch) {
            case KEY_LEFT:
                form_driver(form, REQ_PREV_CHAR);
                break;
            case KEY_RIGHT:
                form_driver(form, REQ_NEXT_CHAR);
                break;
            case KEY_BACKSPACE:
            case KEY_ALTERNATE_BACKSPACE:
                form_driver(form, REQ_PREV_CHAR);
            case KEY_DC:
                form_driver(form, REQ_DEL_CHAR);
                break;
            default:
                form_driver(form, ch);
                break;
        }
    }

    /* final handling */

    if (ch == '\n') {
        form_driver(form, REQ_NEXT_FIELD);
        evt->callback_success(str_strip(field_buffer(field[0], 0)), dptr);
    } else {
        evt->callback_error(dptr);
    }

    unpost_form(form);
    delwin(form_sub(form));
    free_form(form);
    free_field(field[0]);
    delwin(dialog);
}

static void readinput_event_deleter(event_t *ptr) {
    readinput_event_t *evt = (readinput_event_t *) ptr;
    free(evt->prompt);
    free(evt);
}

readinput_event_t *new_readinput_event(char *prompt, bool passwd,
        void (*callback_success)(char *, void *), 
        void (*callback_error)(void *)) {
    readinput_event_t *evt = calloc(1, sizeof(readinput_event_t));
    evt->e_hdr.type = READINPUT_EVENT_TYPE;
    evt->prompt = prompt;
    evt->passwd = passwd;
    evt->callback_success = callback_success;
    evt->callback_error = callback_error;
    return evt;
}

void gui_init(gui_data_t *data) {
    event_loop_init(&data->event_loop);
    ts_map_insert(&data->event_loop.handlers, MESSAGE_EVENT_TYPE,
            message_event_handler);
    ts_map_insert(&data->event_loop.deleters, MESSAGE_EVENT_TYPE,
            message_event_deleter);
    ts_map_insert(&data->event_loop.handlers, READINPUT_EVENT_TYPE,
            readinput_event_handler);
    ts_map_insert(&data->event_loop.deleters, READINPUT_EVENT_TYPE,
            readinput_event_deleter);

    setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();

    data->messages_pad = newpad(MESSAGE_PAD_HEIGHT, MESSAGE_PAD_WIDTH);
    wmove(data->messages_pad, MESSAGE_PAD_HEIGHT - 1, 0);

    gui_init_windows(data);
}

void gui_destroy(gui_data_t *data) {
    gui_kill_windows(data);
    delwin(data->messages_pad);
    
    endwin();
    event_loop_destroy(&data->event_loop);
}

void *gui_thread(void *raw_ptr) {
    gui_data_t *data = (gui_data_t *) raw_ptr;

    int ch;
    bool work = true;
    while ((ch = wgetch(data->input_inner)) != KEY_F(1) && work) {
        if (ch != ERR) {
            switch (ch) {
                case KEY_DOWN:
                    form_driver(data->input_form, REQ_NEXT_LINE);
                    break;
                case KEY_UP:
                    form_driver(data->input_form, REQ_PREV_LINE);
                    break;
                case KEY_LEFT:
                    form_driver(data->input_form, REQ_PREV_CHAR);
                    break;
                case KEY_RIGHT:
                    form_driver(data->input_form, REQ_NEXT_CHAR);
                    break;
                case KEY_NPAGE:
                    ++data->line;
                    data->redraw_messages = true;
                    break;
                case KEY_PPAGE:
                    --data->line;
                    data->redraw_messages = true;
                    break;
                case KEY_RESIZE:
                    gui_restart_windows(data);
                    break;
                case KEY_BACKSPACE:
                case KEY_ALTERNATE_BACKSPACE:
                    form_driver(data->input_form, REQ_PREV_CHAR);
                case KEY_DC:
                    form_driver(data->input_form, REQ_DEL_CHAR);
                    break;
                case KEY_ENTER: {
                        form_driver(data->input_form, REQ_NEXT_FIELD);
                        char *s = str_strip(field_buffer(data->input_field[0], 0));
                        wprintw(data->messages_pad, "=====\n%s\n", s);
                        free(s);
                        data->redraw_messages = true;
                        form_driver(data->input_form, REQ_CLR_FIELD);
                        break; 
                    }
                default:
                    form_driver(data->input_form, ch);
                    break;
            }
        }

        work = event_loop_iteration(&data->event_loop, data);

        if (data->redraw_messages) {
            data->redraw_messages = false;
            data->line = max(0, min(MESSAGE_PAD_HEIGHT -
                        data->messages_height, data->line));
            prefresh(data->messages_pad, data->line, 0, 
                    getbegy(data->messages) + 1, getbegx(data->messages) + 1,
                    getmaxy(data->messages) - 2, getmaxx(data->messages) - 1);
        }

        usleep(10000); // 10 ms
    }

    pthread_exit(NULL);
}

message_t *new_regular_message(char *author, char *msg, struct timeval tv) {
    return new_message(EMT_REGULAR, author, msg, tv);
}

message_t *new_internal_message(char *msg, struct timeval tv) {
    return new_message(EMT_INTERNAL, NULL, msg, tv);
}

message_t *new_server_message(char *msg, struct timeval tv) {
    return new_message(EMT_SERVER, NULL, msg, tv);
}

message_t *new_message(msg_type_t type, char *author, char *msg,
        struct timeval tv) {
    message_t *m = (message_t *)calloc(1, sizeof(message_t));
    m->type = type;
    m->author = author;
    m->msg = msg;
    m->tv = tv;
    return m;
}

void free_message(message_t *msg) {
    if (msg->author != NULL) {
        free(msg->author);
    }
    free(msg->msg);
    free(msg);
}


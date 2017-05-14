#include <client/gui.h>
#include <locale.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <client/events/readinput_event.h>
#include <client/parsed_message.h>
#include <client/misc.h>
#include <client/events/display_parsed_message_job.h>
#include <client/events/send_message_to_server_job.h>
#include <client/defines.h>
#include <client/events/hangup_event.h>
#include <pair.h>
#include <common/message.h>
#include <client/controller.h>

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
            "Xenon chat v0.1 @ Press F10 to exit, F1 to print help and Enter to send message");

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

void gui_print_message(gui_data_t *data, parsed_message_t *msg) {
    wattr_on(data->messages_pad, A_BOLD, NULL);
    const char *author = NULL;
    switch (msg->type) {
        case MESSAGE_SERVER_REGULAR:
        case MESSAGE_SERVER_HISTORY:
            author = msg->author;
            break;
        case MESSAGE_SERVER_META:
            author = "<SERVER>";
            break;
        case MESSAGE_CLIENT_INTERNAL:
            author = "<INTERNAL>";
            break;
        case MESSAGE_SERVER_LIST:
            author = "<USERLIST>";
            break;
    }
    struct timeval tv = parse_tstamp(msg->tstamp);
    wprintw(data->messages_pad, "%s, %s", author, ctime(&tv.tv_sec));
    wattr_off(data->messages_pad, A_BOLD, NULL);
    wprintw(data->messages_pad, "%s\n", msg->text);
}

void gui_init(gui_data_t *data) {
    event_loop_init(&data->event_loop);
    ts_map_insert(&data->event_loop.handlers, DISPLAY_PARSED_MESSAGE_JOB_TYPE,
            display_parsed_message_job_handler);
    ts_map_insert(&data->event_loop.deleters, DISPLAY_PARSED_MESSAGE_JOB_TYPE,
            display_parsed_message_job_deleter);
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

static void get_kick_reason_success(char *s, void *ptr) {
    pair_t *p = ptr;
    gui_data_t *data = p->first;
    char *uid_str = p->second;
    int uid = atoi(uid_str);
    free(uid_str);

    ts_vector_t *tokens = calloc(1, sizeof(ts_vector_t));
    ts_vector_init(tokens, sizeof(message_token_t));
    message_token_t token;

    token.type = DATA_INT32;
    token.data.i32 = uid;
    ts_vector_push_back(tokens, &token);

    token.type = DATA_C_STR;
    token.data.c_str = s;
    ts_vector_push_back(tokens, &token);

    message_t *msg = new_message(MESSAGE_CLIENT_KICK, tokens);
    send_message(msg, data->controller->sockid);
    delete_message(msg);
    free(p);
}

static void get_kick_reason_failure(void *ptr) {
    pair_t *p = ptr;
    free(p->second);
    free(p);
}

static void get_kick_uid_callback(char *s, void *ptr) {
    gui_data_t *data = ptr;
    pair_t *p = calloc(1, sizeof(pair_t));
    p->first = data;
    p->second = s;
    send_event(&data->event_loop, (event_t *) new_readinput_event(
            strdup("Enter kicking reason:"), false, p, get_kick_reason_success, get_kick_reason_failure));
}

void *gui_thread(void *raw_ptr) {
    gui_data_t *data = (gui_data_t *) raw_ptr;

    int ch;
    bool work = true;
    while ((ch = wgetch(data->input_inner)) != KEY_CLOSE_APP && work) {
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
                case '\n': {
                    form_driver(data->input_form, REQ_NEXT_FIELD);
                    char *s = str_strip(field_buffer(data->input_field[0], 0));
                    form_driver(data->input_form, REQ_CLR_FIELD);
                    send_event(data->controller_event_loop, (event_t *) new_send_message_to_server_job(s));
                }
                    break;
                case KEY_PRINT_HELP: {
                    send_event(&data->event_loop, (event_t *) new_display_parsed_message_job(new_parsed_message(
                            MESSAGE_CLIENT_INTERNAL, NULL, strdup(HELP_MSG), get_tstamp()
                    )));
                }
                    break;
                case KEY_LIST_USERS: {
                    ask_for_userlist(data->controller);
                }
                    break;
                case KEY_KICK_USERS: {
                    send_event(&data->event_loop, (event_t *) new_readinput_event(
                            strdup("Enter UID:"), false, data, get_kick_uid_callback, NULL));
                }
                    break;
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

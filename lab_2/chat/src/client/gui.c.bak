#include <client/gui.h>
#include <locale.h>
#include <unistd.h>

#define KEY_ALTERNATE_BACKSPACE 127

#define INPUT_WINDOW_HEIGHT 10

void gui_init(gui_data_t *data) {
    setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();

    getmaxyx(stdscr, data->scr_height, data->scr_width);

    data->statusbar = newwin(1, data->scr_width, data->scr_height - 1, 0);

    data->input = newwin(INPUT_WINDOW_HEIGHT, data->scr_width,
            getbegy(data->statusbar) - INPUT_WINDOW_HEIGHT, 0);
    box(data->input, 0, 0);
    data->input_inner = derwin(data->input, getmaxy(data->input) - 2,
            getmaxx(data->input) - 2, 1, 1);

    data->messages = newwin(getbegy(data->input), data->scr_width, 0, 0);
    box(data->messages, 0, 0);
    data->messages_inner = derwin(data->messages,
            getmaxy(data->messages) - 2, getmaxx(data->messages) - 2, 1, 1);


    keypad(data->input_inner, TRUE);
    nodelay(data->input_inner, TRUE);
    idlok(data->messages_inner, TRUE);
    scrollok(data->messages_inner, TRUE);

    data->input_field[0] = new_field(getmaxy(data->input_inner),
            getmaxx(data->input_inner), 0, 0, 0, 0);
    data->input_field[1] = NULL;
    
    set_field_back(data->input_field[0], A_UNDERLINE);
    field_opts_off(data->input_field[0], O_AUTOSKIP);

    data->input_form = new_form(data->input_field);

    set_form_win(data->input_form, data->input);
    set_form_sub(data->input_form, data->input_inner);

    post_form(data->input_form);

    wprintw(data->statusbar, "Xenon chat v0.1 @ Press F1 to exit, Shift+Enter to send message");
    ts_queue_init(&data->unprinted_messages);

    wrefresh(data->messages);
    wrefresh(data->messages_inner);
    wrefresh(data->input);
    wrefresh(data->input_inner);
    wrefresh(data->statusbar);

    wmove(data->input_inner, 0, 0);
}

void gui_destroy(gui_data_t *data) {
    unpost_form(data->input_form);
    free_form(data->input_form);
    free_field(data->input_field[0]);

    delwin(data->messages);
    delwin(data->messages_inner);
    delwin(data->input);
    delwin(data->input_inner);
    delwin(data->statusbar);

    ts_queue_destroy(&data->unprinted_messages, NULL); /* FIXME */

    endwin();
}

void *gui_thread(void *raw_ptr) {
    gui_data_t *data = (gui_data_t *) raw_ptr;

    int ch;
    while ((ch = wgetch(data->input_inner)) != KEY_F(1)) {
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
                case KEY_BACKSPACE:
                case KEY_ALTERNATE_BACKSPACE:
                    form_driver(data->input_form, REQ_PREV_CHAR);
                    form_driver(data->input_form, REQ_DEL_CHAR);
                    break;
                case KEY_ENTER:
                    form_driver(data->input_form, REQ_NEXT_FIELD);
                    wprintw(data->messages_inner, "=====\n%s\n", field_buffer(data->input_field[0], 0));
                    wrefresh(data->messages_inner);
                    form_driver(data->input_form, REQ_CLR_FIELD);
                    break;
                default:
                    form_driver(data->input_form, ch);
                    break;
            }
        }

//        wrefresh(data->input_inner);
//        wrefresh(data->messages_inner);
//        wrefresh(data->statusbar);
        usleep(10000);
    }

    pthread_exit(NULL);
}

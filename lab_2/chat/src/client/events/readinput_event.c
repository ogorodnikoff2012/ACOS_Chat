//
// Created by xenon on 13.05.17.
//

#include <client/events/readinput_event.h>
#include <client/gui.h>
#include <client/misc.h>
#include <stdlib.h>

void readinput_event_handler(event_t *ptr, void *dptr) {
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
    wmove(dialog, field_y + 1, field_x + 1);
    wrefresh(dialog);

    /* keyboard handling */

    int ch;
    while ((ch = wgetch(dialog)) != '\n' && ch != KEY_CLOSE_APP) {
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
        evt->callback_success(str_strip(field_buffer(field[0], 0)), evt->env);
    } else {
        if (evt->callback_error != NULL) {
            evt->callback_error(evt->env);
        }
    }

    unpost_form(form);
    delwin(form_sub(form));
    free_form(form);
    free_field(field[0]);
    delwin(dialog);
    data->redraw_messages = true;
}

void readinput_event_deleter(event_t *ptr) {
    readinput_event_t *evt = (readinput_event_t *) ptr;
    free(evt->prompt);
    free(evt);
}

readinput_event_t *new_readinput_event(char *prompt, bool passwd, void *env,
                                       void (*callback_success)(char *, void *),
                                       void (*callback_error)(void *)) {
    readinput_event_t *evt = calloc(1, sizeof(readinput_event_t));
    evt->e_hdr.type = READINPUT_EVENT_TYPE;
    evt->prompt = prompt;
    evt->passwd = passwd;
    evt->env = env;
    evt->callback_success = callback_success;
    evt->callback_error = callback_error;
    return evt;
}


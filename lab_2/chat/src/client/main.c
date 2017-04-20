#include <client/gui.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

static void readinput_callback_success(char *str, void *ptr) {
    gui_data_t *data = (gui_data_t *) ptr;
    struct timeval tv;
    
    gettimeofday(&tv, NULL);
    send_event(&data->event_loop, (event_t *) new_message_event(
                new_regular_message(strdup("Callback"), str, tv)));
}

static void readinput_callback_error(void *ptr) {
    gui_data_t *data = (gui_data_t *) ptr;
    send_event(&data->event_loop, &EXIT_EVT);
}

int main() {
    gui_data_t gui_data;
    gui_data.controller_event_loop = NULL;
    
    gui_init(&gui_data);
    struct timeval tv;
    
    pthread_t gui;
    pthread_create(&gui, NULL, gui_thread, &gui_data);
    
    gettimeofday(&tv, NULL);
    send_event(&gui_data.event_loop, (event_t *) new_message_event(new_regular_message(strdup("Xenon"), strdup("Тестовое сообщение #1"), tv)));

    usleep(1e6); // 1s

    gettimeofday(&tv, NULL);
    send_event(&gui_data.event_loop, (event_t *) new_message_event(new_regular_message(strdup("Xenon"), strdup("Тестовое сообщение #2"), tv)));

    usleep(1e6); // 1s
    send_event(&gui_data.event_loop, (event_t *) new_readinput_event(
                strdup("Password"), true, readinput_callback_success,
                readinput_callback_error));

    pthread_join(gui, NULL);

    gui_destroy(&gui_data);
    pthread_exit(NULL);
}

#include <client/gui.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <client/controller.h>
#include <client/listener.h>

int main(int argc, char *argv[]) {

    if (argc > 1) {
        printf("Working in debugging mode\n");
        int x;
        scanf("%d", &x);
    }
    gui_data_t gui_data;
    client_controller_data_t controller_data;
    client_listener_data_t listener_data;

    gui_data.controller_event_loop = &controller_data.event_loop;
    controller_data.gui_event_loop = &gui_data.event_loop;
    gui_data.controller = &controller_data;
    listener_data.controller = &controller_data;

    
    gui_init(&gui_data);
    client_controller_init(&controller_data);
    client_listener_init(&listener_data);

    pthread_t gui, controller, listener;
    pthread_create(&gui, NULL, gui_thread, &gui_data);
    pthread_create(&controller, NULL, client_controller_thread, &controller_data);
    pthread_create(&listener, NULL, client_listener_thread, &listener_data);

    pthread_join(gui, NULL);

    send_urgent_event(&controller_data.event_loop, &EXIT_EVT);
    pthread_join(controller, NULL);

    send_urgent_event(&listener_data.event_loop, &EXIT_EVT);
    pthread_join(listener, NULL);

    gui_destroy(&gui_data);
    client_controller_destroy(&controller_data);
    client_listener_destroy(&listener_data);

    pthread_exit(NULL);
}

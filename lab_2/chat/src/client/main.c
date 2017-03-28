#include <client/gui.h>
#include <pthread.h>

int main() {
    gui_data_t gui_data;
    gui_data.controller_event_loop = NULL;
    
    gui_init(&gui_data);
    pthread_t gui;
    pthread_create(&gui, NULL, gui_thread, &gui_data);
    
    pthread_join(gui, NULL);

    gui_destroy(&gui_data);
    pthread_exit(NULL);
}

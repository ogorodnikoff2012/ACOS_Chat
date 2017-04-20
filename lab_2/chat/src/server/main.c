#include <server/controller.h>
#include <server/listener.h>

int main() {
    controller_data_t cdata;
    controller_init(&cdata);

    listener_data_t ldata;
    ldata.controller_event_loop = &cdata.event_loop;
    listener_init(&ldata);

    pthread_t controller, listener;
    pthread_create(&controller, NULL, controller_thread, &cdata);
    pthread_create(&listener, NULL, listener_thread, &ldata);

    pthread_join(controller, NULL);
    pthread_join(listener, NULL);

    controller_destroy(&cdata);
    listener_destroy(&ldata);

    pthread_exit(NULL);
}

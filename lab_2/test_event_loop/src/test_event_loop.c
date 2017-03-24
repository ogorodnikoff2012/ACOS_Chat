#include <pthread.h>
#include <event_loop/event_loop.h>
#include <stdio.h>
#include <stdlib.h>

event_loop_t loop;

#define NUMBER_EVENT_TYPE 1

typedef struct number_event {
    int type, data;
} number_event_t;

event_t *new_number_event(int x) {
    number_event_t *evt = calloc(1, sizeof(number_event_t));
    evt->type = NUMBER_EVENT_TYPE;
    evt->data = x;
    return (event_t *)evt;
}

void number_event_handler(event_t *evt) {
    number_event_t *e = (number_event_t *)evt;
    printf("Recieved %d\n", e->data);
    if (e->data == 0) {
        printf("Exiting\n");
        send_event(&loop, &EXIT_EVT);
    }
}

void *printer_main(void *ptr) {
    ts_map_insert(&loop.handlers, NUMBER_EVENT_TYPE, number_event_handler);
    run_event_loop(&loop);
    event_loop_destroy(&loop);
    pthread_exit(NULL);
}

int main() {
    pthread_t printer_thread;
    int rc;

    event_loop_init(&loop);
    rc = pthread_create(&printer_thread, NULL, printer_main, NULL);
    if (rc) {
        printf("ERROR %d\n", rc);
        exit(-1);
    }

    printf("I can echo integers\nType 0 or <EOF> to exit\n");

    int x = 1;
    rc = 0;
    while (x && rc != EOF) {
        rc = scanf("%d", &x);
        if (rc != EOF) {
            send_event(&loop, new_number_event(x));
        }
    }

    if (rc == EOF) {  
        printf("Exiting\n");
        send_event(&loop, &EXIT_EVT);
    }

    pthread_join(printer_thread, NULL);
    pthread_exit(NULL);
}

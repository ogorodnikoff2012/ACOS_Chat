#include <ncurses6/ncursesw/ncurses.h>
#include <locale.h>
#include <pthread.h>
#include <event_loop/event_loop.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#define NETWORK_EVENT_TYPE 1
#define KEYBOARD_EVENT_TYPE 2

#define BUFFER_SIZE 80

#define GUI_THREAD 0
#define DATA_THREAD 1
#define NETWORK_INPUT_THREAD 2
#define NETWORK_OUTPUT_THREAD 3
#define N_THREADS 4

event_loop_t *event_loops[N_THREADS];
pthread_t threads[N_THREADS];

typedef struct network_event {
    event_t e_hdr;
    char *msg;
} network_event_t;

typedef struct keyboard_event {
    event_t e_hdr;
    int key;
} keyboard_event_t;

typedef struct msg_buf {
    char **data;
    int entry_num, capacity;
    pthread_mutex_t mutex;
} msg_buf_t;

void send_event_broadcast(event_t *e) {
    for (int i = 0; i < N_THREADS; ++i) {
        if (event_loops[i] != NULL) {
            send_event(event_loops[i], e);
        }
    }
}

void network_event_handler(event_t *e, void *data) {
    network_event_t *evt = (network_event_t *)e;
    msg_buf_t *buf = (msg_buf_t *)data;
    
    pthread_mutex_lock(&buf->mutex);
    if (buf->entry_num == buf->capacity) {
        free(buf->data[0]);
        for (int i = 1; i < buf->entry_num; ++i) {
            buf->data[i - 1] = buf->data[i];
        }
        --buf->entry_num;
    }
    buf->data[buf->entry_num] = malloc(2 * BUFFER_SIZE);
    int len = sprintf(buf->data[buf->entry_num], "Recieved msg: %.*s", BUFFER_SIZE, evt->msg);

    for (int i = 0; i < len; ++i) {
        if (buf->data[buf->entry_num][i] == '\r') {
            buf->data[buf->entry_num][i] = 0;
        }
    }

    buf->data[buf->entry_num][len] = 0;
    ++buf->entry_num;
    pthread_mutex_unlock(&buf->mutex);
}

void network_event_deleter(event_t *e) {
    network_event_t *evt = (network_event_t *)e;
    free(evt->msg);
    free(evt);
}

network_event_t *new_network_event(char *msg) {
    network_event_t *evt = calloc(1, sizeof(network_event_t));
    evt->e_hdr.type = NETWORK_EVENT_TYPE;
    evt->msg = msg;
    return evt;
}

void keyboard_event_handler(event_t *e, void *data) {
    static char buffer[BUFFER_SIZE];
    keyboard_event_t *evt = (keyboard_event_t *)e;
    int sockid = *(int *)data;

    int len = sprintf(buffer, "Key pressed: %d\n", evt->key);
    int count = send(sockid, buffer, len, 0);
    if (count == -1) {
        send_event_broadcast(&EXIT_EVT);
    }
}

keyboard_event_t *new_keyboard_event(int key) {
    keyboard_event_t *evt = calloc(1, sizeof(keyboard_event_t));
    evt->e_hdr.type = KEYBOARD_EVENT_TYPE;
    evt->key = key;
    return evt;
}

void *gui_thread(void *ptr) {
    setlocale(LC_ALL, "");
    initscr();
    noecho();
    nodelay(stdscr, TRUE);
    cbreak();
    keypad(stdscr, TRUE);
    
    msg_buf_t *buf = (msg_buf_t *)ptr;

    int ch;
    while ((ch = getch()) != KEY_F(1)) {
        if (ch != ERR) {
            event_loop_t *loop = event_loops[NETWORK_OUTPUT_THREAD];
            if (loop != NULL) {
                send_event(loop, (event_t *)new_keyboard_event(ch));
            }
        }
        if (pthread_mutex_trylock(&buf->mutex) == 0) {
            move(0, 0);
            for (int i = 0; i < buf->entry_num; ++i) {
//                clrtoeol();
                printw("%s\n", buf->data[i]);
            }
//            refresh();
            pthread_mutex_unlock(&buf->mutex);
        }
    }

    endwin();
    send_event_broadcast(&EXIT_EVT);
    pthread_exit(NULL);
}

void *data_thread(void *ptr) {
    event_loop_t loop;
    event_loop_init(&loop);

    ts_map_insert(&loop.handlers, NETWORK_EVENT_TYPE, network_event_handler);
    ts_map_insert(&loop.deleters, NETWORK_EVENT_TYPE, network_event_deleter);
    event_loops[DATA_THREAD] = &loop;
    
    run_event_loop(&loop, ptr);
    
    event_loops[DATA_THREAD] = NULL;
    event_loop_destroy(&loop);
    send_event_broadcast(&EXIT_EVT);
    pthread_exit(NULL);
}

int prepare_server(int p) {
    int sockid = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockid == -1) {
        return -1;
    }

    struct sockaddr_in port;
    port.sin_family = AF_INET;
    port.sin_port = htons(p);
    port.sin_addr.s_addr = htonl(INADDR_ANY);
    int status = bind(sockid, (struct sockaddr *)&port, sizeof(port));
    if (status == -1) {
        return -1;
    }

    status = listen(sockid, 1);

    if (status == -1) {
        return -1;
    }
    return sockid;
}

void *network_output_thread(void *sockid) {
    event_loop_t loop;
    event_loop_init(&loop);

    ts_map_insert(&loop.handlers, KEYBOARD_EVENT_TYPE, keyboard_event_handler);
    event_loops[NETWORK_OUTPUT_THREAD] = &loop;
    
    run_event_loop(&loop, sockid);
    
    event_loops[NETWORK_OUTPUT_THREAD] = NULL;
    event_loop_destroy(&loop);

    shutdown(*(int *)sockid, SHUT_RDWR);

    pthread_kill(threads[NETWORK_INPUT_THREAD], SIGTERM);
    pthread_exit(NULL);
}

void *network_input_thread(void *not_used) {
    pthread_t output_thread;

    int conn_sockid;
    int sockid = prepare_server(12345);

    if (sockid == -1) {
        send_event_broadcast(&EXIT_EVT);
        pthread_exit(NULL);
    }
    
    struct sockaddr client_addr;
    unsigned int addr_len = sizeof(client_addr);
    int status = accept(sockid, &client_addr, &addr_len);
    
    shutdown(sockid, SHUT_RDWR);
    
    if (status == -1) {
        send_event_broadcast(&EXIT_EVT);
        pthread_exit(NULL);
    }

    conn_sockid = status;
    pthread_create(&output_thread, NULL, network_output_thread, &conn_sockid);
    
    char *buffer = malloc(BUFFER_SIZE + 1);
    while ((status = recv(conn_sockid, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[status] = 0;
        event_loop_t *el = event_loops[DATA_THREAD];
        if (el != NULL) {
            send_event(el, (event_t *)new_network_event(buffer));
            buffer = malloc(BUFFER_SIZE + 1);
        }
//        sleep(1);
    }

    send_event_broadcast(&EXIT_EVT);
    shutdown(conn_sockid, SHUT_RDWR);
    shutdown(sockid, SHUT_RDWR);
    pthread_join(output_thread, NULL);
    pthread_exit(NULL); 
}

int main() {
    int x;
    scanf("%d", &x);   

    msg_buf_t buffer;

    buffer.capacity = 10;
    buffer.entry_num = 0;
    buffer.data = calloc(10, sizeof(char *));

    pthread_mutex_init(&buffer.mutex, NULL);

    pthread_create(&threads[GUI_THREAD], NULL, gui_thread, &buffer);
    pthread_create(&threads[NETWORK_INPUT_THREAD], NULL, network_input_thread, NULL);
    pthread_create(&threads[DATA_THREAD], NULL, data_thread, &buffer);

    pthread_join(threads[GUI_THREAD], NULL);
    pthread_join(threads[NETWORK_INPUT_THREAD], NULL);
    pthread_join(threads[DATA_THREAD], NULL);

    for (int i = 0; i < buffer.entry_num; ++i) {
        free(buffer.data[i]);
    }
    free(buffer.data);

    pthread_exit(NULL);
}

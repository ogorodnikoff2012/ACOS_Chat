#include <server/controller.h>
#include <server/listener.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <server/db.h>
#include <server/events/change_worker_cnt_event.h>

static int prepare_conn_mgr_callback(void *ptr, int argc, char *argv[], char *col_name[]) {
    conn_mgr_t *mgr = ptr;
    if (argc > 0) {
        conn_mgr_add_uid(mgr, atoi(argv[0]));
        return 0;
    }
    return 1;
}

static void prepare_conn_mgr(sqlite3 *db, conn_mgr_t *mgr) {
    char *errmsg = NULL;
    sqlite3_exec(db, "SELECT uid FROM users;", prepare_conn_mgr_callback, mgr, &errmsg);
}

int main() {
    controller_data_t cdata;
    controller_init(&cdata);

    listener_data_t ldata;
    listener_init(&ldata);

    cdata.listener_event_loop = &ldata.event_loop;
    ldata.controller_event_loop = &cdata.event_loop;
    ldata.conn_mgr = &cdata.conn_mgr;

    prepare_db(cdata.db);
    prepare_conn_mgr(cdata.db, &cdata.conn_mgr);

    pthread_t controller, listener;
    pthread_create(&controller, NULL, controller_thread, &cdata);
    pthread_setname_np(controller, "controller");
    pthread_create(&listener, NULL, listener_thread, &ldata);
    pthread_setname_np(listener, "listener");

    send_event(&cdata.event_loop, (event_t *) new_change_worker_cnt_event(5));

    printf("Server is ready. To stop it, press Ctrl+D (send EOF).\n");

    while (!feof(stdin)) {
        char c = '\0';
        fread(&c, 1, 1, stdin);
        if (c == '+') {
            send_event(&cdata.event_loop, (event_t *) new_change_worker_cnt_event(cdata.workers_cnt + 1));
        } else if (c == '-') {
            int new_workers_cnt = cdata.workers_cnt - 1;
            if (new_workers_cnt > 0) {
                send_event(&cdata.event_loop, (event_t *) new_change_worker_cnt_event(new_workers_cnt));
            }
        } else if (c == '?') {
            printf("Now has %d worker%s.\n", cdata.workers_cnt, (cdata.workers_cnt == 1 ? "" : "s"));
        }
        usleep(10000);
    }

    send_event(&cdata.event_loop, &EXIT_EVT);
    send_event(&ldata.event_loop, &EXIT_EVT);

    pthread_join(controller, NULL);
    pthread_join(listener, NULL);

    controller_destroy(&cdata);
    listener_destroy(&ldata);

    pthread_exit(NULL);
}

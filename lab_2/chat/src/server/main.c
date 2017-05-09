#define _GNU_SOURCE
#include <server/controller.h>
#include <server/listener.h>
#include <unistd.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <logger.h>
#include <xenon_md5/xenon_md5.h>

static int count_callback(void *ptr, int n, char **col, char **names) {
    *(int *) ptr = n;
    return 0;
}

static void prepare_db(sqlite3 *db) {
    char *errmsg;
    int rc;
#ifdef DEBUG
    rc = sqlite3_exec(db, DB_RESET, NULL, NULL, &errmsg);
    LOG("Database dropped, rc = %d, errmsg = %s", rc, errmsg);
#endif
    rc = sqlite3_exec(db, DB_INIT, NULL, NULL, &errmsg);
    LOG("Database prepared, rc = %d, errmsg = %s", rc, errmsg);
    
    int n = 0;
    rc = sqlite3_exec(db, "SELECT * FROM users WHERE login='root';", count_callback, &n, &errmsg);
    LOG("'root' user search: success = %d, rc = %d, errmsg = %s", n, rc, errmsg);

    if (n == 0) {
        char *passwd = getpass("Enter password for 'root' user: ");
        const char *tail = NULL;
        sqlite3_stmt *stmt;
        rc = sqlite3_prepare(db, "INSERT INTO users VALUES (?001, ?002, ?003);", -1, &stmt, &tail);
        sqlite3_bind_int(stmt, 1, 1); /* root UID = 1 */
        sqlite3_bind_text(stmt, 2, "root", -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, md5hex(passwd), -1, free);

        while ((rc = sqlite3_step(stmt)) != SQLITE_DONE) {
            if (rc == SQLITE_ERROR) {
                LOG("Fucking error while adding 'root' user");
                exit(-1);
            }
        }
        sqlite3_finalize(stmt);
        free(passwd);
    }
}

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
        char c;
        fread(&c, 1, 1, stdin);
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

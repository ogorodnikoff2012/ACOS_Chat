//
// Created by xenon on 09.05.17.
//

#include <server/db.h>
#include <xenon_md5/xenon_md5.h>
#include <string.h>
#include <server/defines.h>
#include <stdlib.h>
#include <unistd.h>
#include <common/logger.h>
#include <server/misc.h>

int login_or_register(sqlite3 *db, char *login, char *passwd) {
    char *hash = md5hex(passwd);
    int rc;
    const char *tail = NULL;
    sqlite3_stmt *stmt;

    sqlite3_prepare(db, "SELECT uid, password_hash FROM users WHERE login = ?001;",
                    -1, &stmt, &tail);
    sqlite3_bind_text(stmt, 1, login, -1, SQLITE_STATIC);

    int rows = 0;
    int uid = -MSG_STATUS_AUTH_ERROR;

    while ((rc = sqlite3_step(stmt)) != SQLITE_DONE) {
        while (rc == SQLITE_BUSY) {
            usleep(100); /* 100 us */
            rc = sqlite3_step(stmt);
        }
        if (rc != SQLITE_ROW) {
            sqlite3_finalize(stmt);
            free(hash);
            return -MSG_STATUS_AUTH_ERROR;
        }
        ++rows;
        if (strcmp(hash, (const char*)sqlite3_column_text(stmt, 1)) == 0) {
            uid = sqlite3_column_int(stmt, 0);
        }
    }
    sqlite3_finalize(stmt);

    if (rows == 0) {
        sqlite3_prepare(db, "SELECT MAX(uid) + 1 FROM USERS;", -1, &stmt, &tail);
        while ((rc = sqlite3_step(stmt)) != SQLITE_DONE) {
            while (rc == SQLITE_BUSY) {
                usleep(100); /* 100 us */
                rc = sqlite3_step(stmt);
            }
            if (rc != SQLITE_ROW) {
                sqlite3_finalize(stmt);
                free(hash);
                return -MSG_STATUS_REG_ERROR;
            }
            uid = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);

        sqlite3_prepare(db, "INSERT INTO users VALUES (?001, ?002, ?003);", -1, &stmt, &tail);
        sqlite3_bind_int(stmt, 1, uid);
        sqlite3_bind_text(stmt, 2, login, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, hash, -1, SQLITE_STATIC);

        while ((rc = sqlite3_step(stmt)) != SQLITE_DONE) {
            if (rc == SQLITE_ERROR) {
                sqlite3_finalize(stmt);
                return -MSG_STATUS_REG_ERROR;
            }
        }
        sqlite3_finalize(stmt);
    }

    free(hash);
    return uid;
}

char *get_login_by_uid(sqlite3 *db, int uid) {
    sqlite3_stmt *stmt;
    const char *tail;
    sqlite3_prepare(db, "SELECT login FROM users WHERE uid = ?001;", -1, &stmt, &tail);
    sqlite3_bind_int(stmt, 1, uid);

    int rc;
    char *ans = NULL;
    while ((rc = sqlite3_step(stmt)) != SQLITE_DONE) {
        while (rc == SQLITE_BUSY) {
            usleep(100); /* 100 us */
            rc = sqlite3_step(stmt);
        }
        if (rc != SQLITE_ROW) {
            sqlite3_finalize(stmt);
            return NULL;
        }
        if (ans != NULL) {
            free(ans);
        }
        ans = strdup((const char*)sqlite3_column_text(stmt, 0));
    }
    sqlite3_finalize(stmt);
    return ans;
}

static int count_callback(void *ptr, int n, char **col, char **names) {
    *(int *) ptr = n;
    return 0;
}

void prepare_db(sqlite3 *db) {
    char *errmsg;
    int rc;
#ifdef RESET_DB
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
        sqlite3_bind_int(stmt, 1, ROOT_UID);
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

void browse_history(sqlite3 *db, int cnt, void *env, void (* callback)(void *, sqlite3_stmt *)) {
    sqlite3_stmt *stmt;
    const char *tail;
    sqlite3_prepare(db, "SELECT * FROM (SELECT * FROM messages ORDER BY tstamp DESC "
            "LIMIT ?001) AS t ORDER BY tstamp;", -1, &stmt, &tail);
    sqlite3_bind_int(stmt, 1, to_range(0, cnt, MAX_HISTORY_LENGTH));

    int rc;
    while ((rc = sqlite3_step(stmt)) != SQLITE_DONE) {
        while (rc == SQLITE_BUSY) {
            usleep(100); /* 100 us */
            rc = sqlite3_step(stmt);
        }
        if (rc != SQLITE_ROW) {
            sqlite3_finalize(stmt);
        }
        callback(env, stmt);
    }
    sqlite3_finalize(stmt);
}

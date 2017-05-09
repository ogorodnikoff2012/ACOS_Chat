//
// Created by xenon on 09.05.17.
//

#ifndef XENON_CHAT_SERVER_DB_H
#define XENON_CHAT_SERVER_DB_H

#include <sqlite3.h>

int login_or_register(sqlite3 *db, char *login, char *passwd);
char *get_login_by_uid(sqlite3 *db, int uid);
void prepare_db(sqlite3 *db);
void browse_history(sqlite3 *db, int cnt, void *env, void (* callback)(void *, sqlite3_stmt *));

#endif // XENON_CHAT_SERVER_DB_H

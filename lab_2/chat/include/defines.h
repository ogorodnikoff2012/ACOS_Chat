#ifndef XENON_CHAT_CLIENT_DEFINES_H
#define XENON_CHAT_CLIENT_DEFINES_H

#define GUI_THREAD_ID 1
#define CONTROLLER_THREAD_ID 2
#define LISTENER_THREAD_ID 3
#define WORKER_THREAD_ID 4

#define MSG_HEADER_SIZE 5

#define MSG_STATUS_OK 0
#define MSG_STATUS_INVALID_TYPE 1
#define MSG_STATUS_UNAUTHORIZED 2
#define MSG_STATUS_AUTH_ERROR 3
#define MSG_STATUS_REG_ERROR 4
#define MSG_STATUS_ACCESS_ERROR 5
#define MSG_STATUS_INVALID_MSG 6

#define DB_FILENAME "xenon_chat_server.sqlite"

#define DB_RESET    "DROP TABLE IF EXISTS users;" \
                    "DROP TABLE IF EXISTS conns;"

#define DB_SCHEMA   "CREATE TABLE IF NOT EXISTS users (id integer primary key, login nvarchar(32)," \
                                                        "password_hash varchar(32));" \
                    "CREATE TABLE IF NOT EXISTS conns (uid integer, sockid integer primary key);"

#endif /* XENON_CHAT_CLIENT_DEFINES_H */

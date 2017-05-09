#ifndef XENON_CHAT_CLIENT_DEFINES_H
#define XENON_CHAT_CLIENT_DEFINES_H

#define GUI_THREAD_ID 1
#define CONTROLLER_THREAD_ID 2
#define LISTENER_THREAD_ID 3
#define WORKER_THREAD_ID 4

#define BROADCAST_MESSAGE_EVENT_TYPE 1
#define CHANGE_WORKER_CNT_EVENT_TYPE 2
#define CLOSE_CONNECTION_EVENT_TYPE 3
#define INPUT_MSG_EVENT_TYPE 4
#define PROCESS_MESSAGE_JOB_TYPE 5
#define SEND_MESSAGE_JOB_TYPE 6
#define WORKER_EXIT_EVENT_TYPE 7

#define MSG_HEADER_SIZE 5

#define MSG_STATUS_OK 0
#define MSG_STATUS_INVALID_TYPE 1
#define MSG_STATUS_UNAUTHORIZED 2
#define MSG_STATUS_AUTH_ERROR 3
#define MSG_STATUS_REG_ERROR 4
#define MSG_STATUS_ACCESS_ERROR 5
#define MSG_STATUS_INVALID_MSG 6

#define MESSAGE_SERVER_REGULAR 'r'
#define MESSAGE_SERVER_META 'm'
#define MESSAGE_SERVER_STATUS 's'
#define MESSAGE_SERVER_HISTORY 'h'

#define MESSAGE_CLIENT_REGULAR 'r'
#define MESSAGE_CLIENT_LOGIN 'i'
#define MESSAGE_CLIENT_LOGOUT 'o'
#define MESSAGE_CLIENT_HISTORY 'h'

#define MAX_HISTORY_LENGTH 50

#define NULL_UID 0

#define DB_FILENAME "xenon_chat_server.sqlite"

#define DB_RESET    "DROP TABLE IF EXISTS users;" \
                    "DROP TABLE IF EXISTS messages;"

#define DB_SCHEMA   "CREATE TABLE IF NOT EXISTS users (uid integer primary key, login nvarchar(32)," \
                                                        "password_hash varchar(32));" \
                    "CREATE TABLE IF NOT EXISTS messages (uid integer, msg text, tstamp bigint);"

#define DB_INIT     DB_SCHEMA

#endif /* XENON_CHAT_CLIENT_DEFINES_H */

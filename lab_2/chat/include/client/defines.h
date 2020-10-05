#ifndef XENON_CHAT_CLIENT_DEFINES_H
#define XENON_CHAT_CLIENT_DEFINES_H

#define KEY_ESCAPE 27
#define KEY_ALTERNATE_BACKSPACE 127
#define KEY_CLOSE_APP KEY_F(10)
#define KEY_LIST_USERS KEY_F(5)
#define KEY_PRINT_HELP KEY_F(1)
#define KEY_KICK_USERS KEY_F(2)

#define INPUT_WINDOW_HEIGHT 10
#define MESSAGE_PAD_WIDTH 120
#define MESSAGE_PAD_HEIGHT 2000

#define READINPUT_EVENT_TYPE 1
#define DISPLAY_PARSED_MESSAGE_JOB_TYPE 2
#define SEND_MESSAGE_TO_SERVER_JOB_TYPE 3
#define HANGUP_EVENT_TYPE 4
#define KICK_EVENT_TYPE 5
#define LIST_EVENT_TYPE 6
#define STATUS_EVENT_TYPE 7

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
#define MESSAGE_SERVER_LIST 'l'
#define MESSAGE_SERVER_KICK 'k'

#define MESSAGE_CLIENT_REGULAR 'r'
#define MESSAGE_CLIENT_LOGIN 'i'
#define MESSAGE_CLIENT_LOGOUT 'o'
#define MESSAGE_CLIENT_HISTORY 'h'
#define MESSAGE_CLIENT_LIST 'l'
#define MESSAGE_CLIENT_KICK 'k'
#define MESSAGE_CLIENT_INTERNAL '@'

#define MAX_HISTORY_LENGTH 50

#define HELP_MSG "Press F1 to read this help;\n" \
"Press F5 to see list of users;\n" \
"Press F2 to kick user (you can do it only if you're root);\n" \
"Press F10 to quit;\n" \
"Press Enter to send message.\n" \
"Warning! If you see '[[A' when you press <F1>, check your terminal settings.\n" \
"It's possible that this client cannot recognize escape key sequences.\n" \
"More info: https://stackoverflow.com/questions/21843395"

#endif /* XENON_CHAT_CLIENT_DEFINES_H */

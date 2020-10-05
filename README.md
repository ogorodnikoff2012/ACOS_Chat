ACOS_Chat
==========

This project was a part of the course on Architecture of Computers and Operating Systems (1st year, spring semester in MIPT).

Here you can find two applications, a server and a client for a chat. It uses TCP sockets to send and recieve messages.

Features:
- NCurses interface;
- Multithreaded server can process many connections in multiplexing mode;
- Uses asynchronous event-driven model;
- SQLite3 database is used to store chat history;
- Written in pure C (GNU99)!

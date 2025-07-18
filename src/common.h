// Copyright 2025 Michał Jankowski
#ifndef SRC_COMMON_H_
#define SRC_COMMON_H_

extern const char* MCAST4_GROUP;
extern const char* MCAST6_GROUP;
extern const unsigned int PROGRAM_PORT;

extern const unsigned int MAX_POLL_FDS;
extern const unsigned int POLL_TIMEOUT_MS;
extern const char* LOCKFILE_DIR;

extern const size_t MSG_BUFFER_SIZE;
extern const size_t NET_BUFFER_SIZE;  // TODO(.): Figure out what to do with this one

typedef enum {
    CMD_UNKNOWN,
    CMD_SILENT,
    CMD_HELP,
    CMD_EXIT,
    CMD_CLEAR_ALL,
    CMD_SCAN,
    CMD_PRINT_PEERS,
    CMD_SEND,
    CMD_DISCONNECT_ALL,
    CMD_WHOAMI,
} Command;

#endif  // SRC_COMMON_H_

// Copyright 2025 Michał Jankowski
#include <stdlib.h>
#include <stdio.h>

#include "common.h"
#include "command_processor.h"
#include "peers.h"
#include "network.h"
#include "socket_preparation.h"

const unsigned int MAX_POLL_FDS = 5;
const unsigned int POLL_TIMEOUT_MS = 100;
const char* LOCKFILE_DIR = "/var/lock";

const size_t MSG_BUFFER_SIZE = 2048;

int main(int argc, char *argv[]) {
    char stdin_buffer[MSG_BUFFER_SIZE];
    char user_identifier[USER_IDENTIFIER_SIZE];
    CmdReturnSignal cmd_signal;

    if (argc != 3) {
        fprintf(stderr, "Usage: c_comm [INTERFACE NAME] [USER NAME]\n");
        exit(EXIT_FAILURE);
    }

    if (CreateLocalUserIdentifier(argv[2], user_identifier, sizeof(user_identifier)) < 0) {
        fprintf(stderr, "[FAIL] Could not create user identifier. Exiting.\n");
        exit(EXIT_FAILURE);
    }

    printf("Hello!\nThis user/instance will be identified as: \"%s\"\n", user_identifier);
    printf("Type /help for a list of commands or /exit to exit the application.\n\n");
    while (1) {
        fgets(stdin_buffer, sizeof(stdin_buffer), stdin);
        cmd_signal = HandleCommand(stdin_buffer);
        if (cmd_signal == CMD_RETURN_EXIT) {
            printf("Goodbye!\n");
            return 0;
        }
    }
}

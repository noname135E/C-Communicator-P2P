// Copyright 2025 Michał Jankowski
#include <stdlib.h>
#include <stdio.h>

#include "common.h"
#include "command_processor.h"
#include "network.h"
#include "socket_preparation.h"

const unsigned int MAX_POLL_FDS = 5;
const unsigned int POLL_TIMEOUT_MS = 100;
const char* LOCKFILE_DIR = "/var/lock";

const size_t MSG_BUFFER_SIZE = 2048;

int main(int argc, char *argv[]) {
    char stdin_buffer[MSG_BUFFER_SIZE];
    CmdReturnSignal cmd_signal;
    while (1) {
        fgets(stdin_buffer, sizeof(stdin_buffer), stdin);
        cmd_signal = HandleCommand(stdin_buffer);
        if (cmd_signal == CMD_RETURN_EXIT) {
            printf("Goodbye!\n");
            return 0;
        }
    }
}
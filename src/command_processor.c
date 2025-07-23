// Copyright 2025 Michał Jankowski
#include <stdio.h>
#include <string.h>

#include "command_processor.h"

CmdProcessEntry cmd_entries[] = {
    {"?", "prints information about available commands", ProcessCmdHelp},
    {"/help", "prints information about available commands", ProcessCmdHelp},
    {"/exit", "exits application", ProcessCmdExit},
    {NULL, NULL, NULL}
};

CmdReturnSignal HandleCommand(char* input) {
    printf("\n");
    input[strcspn(input, "\n")] = '\0';
    for (size_t i = 0; cmd_entries[i].cmd_string != NULL; i++) {
        const size_t cmd_len = strlen(cmd_entries[i].cmd_string);
        if (strncmp(input, cmd_entries[i].cmd_string, cmd_len) == 0 &&
            (input[cmd_len] == ' ' || input[cmd_len] == '\0')) {
            const char* args_string = input + cmd_len;
            return cmd_entries[i].process_function(args_string);
        }
    }
    printf("Unknown command. See \"/help\" for help.\n");
    return CMD_RETURN_UNKNOWN;
}

CmdReturnSignal ProcessCmdHelp(const char* args_string) {
    (void) args_string;
    for (size_t i = 0; cmd_entries[i].cmd_string != NULL; i++) {
        printf("%s\t - %s\n", cmd_entries[i].cmd_string, cmd_entries[i].cmd_help_string);
    }
    return CMD_RETURN_OK;
}

CmdReturnSignal ProcessCmdExit(const char* args_string) {
    (void) args_string;
    printf("Exiting...\n");
    return CMD_RETURN_EXIT;
}

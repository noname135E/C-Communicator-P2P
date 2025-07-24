// Copyright 2025 Michał Jankowski
#include <stdio.h>
#include <string.h>

#include "command_processor.h"
#include "peers.h"

// TODO(.): Maybe split this up into sections?
CmdProcessEntry cmd_entries[] = {
    {"?", "prints information about available commands", ProcessCmdHelp},
    {"/help", "prints information about available commands", ProcessCmdHelp},
    {"/exit", "exits application", ProcessCmdExit},
    {"/peers", "prints all discovered peers", ProcessCmdPrintPeers},
    {"/whoami", "prints user identifier", ProcessCmdWhoami},
    {NULL, NULL, NULL}
};

CmdReturnSignal HandleCommand(char* input, NetworkContext* net_context) {
    printf("\n");
    input[strcspn(input, "\n")] = '\0';
    for (size_t i = 0; cmd_entries[i].cmd_string != NULL; i++) {
        const size_t cmd_len = strlen(cmd_entries[i].cmd_string);
        if (strncmp(input, cmd_entries[i].cmd_string, cmd_len) == 0 &&
            (input[cmd_len] == ' ' || input[cmd_len] == '\0')) {
            const char* args_string = input + cmd_len;
            return cmd_entries[i].process_function(args_string, net_context);
        }
    }
    printf("Unknown command. See \"/help\" for help.\n");
    return CMD_RETURN_UNKNOWN;
}

CmdReturnSignal ProcessCmdHelp(const char* args_string, NetworkContext* net_context) {
    (void) args_string;
    (void) net_context;
    for (size_t i = 0; cmd_entries[i].cmd_string != NULL; i++) {
        printf("%s\t - %s\n", cmd_entries[i].cmd_string, cmd_entries[i].cmd_help_string);
    }
    return CMD_RETURN_OK;
}

CmdReturnSignal ProcessCmdExit(const char* args_string, NetworkContext* net_context) {
    (void) args_string;
    (void) net_context;
    printf("Exiting...\n");
    return CMD_RETURN_EXIT;
}

CmdReturnSignal ProcessCmdNotImplemented(const char* args_string, NetworkContext* net_context) {
    (void) args_string;
    (void) net_context;
    printf("Oops! This command is not implemented...\n");
    return CMD_RETURN_OK;
}

CmdReturnSignal ProcessCmdWhoami(const char* args_string, NetworkContext* net_context) {
    (void) args_string;
    if (net_context->user_identifier != NULL) {
        printf("You are: \"%s\".\n", net_context->user_identifier);
    } else {
        printf("User identifier is not set.\n");
    }
    return CMD_RETURN_OK;
}

CmdReturnSignal ProcessCmdPrintPeers(const char* args_string, NetworkContext* net_context) {
    (void) args_string;

    if (net_context == NULL || net_context->peers == NULL) {
        fprintf(stderr, "[ ERR] Print peers received NULL pointer.\n");
        return CMD_RETURN_ERROR;
    }
    PrintPeers(net_context->peers, net_context->peers_size);
    return CMD_RETURN_OK;
}

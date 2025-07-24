// Copyright 2025 Michał Jankowski
#ifndef SRC_COMMAND_PROCESSOR_H_
#define SRC_COMMAND_PROCESSOR_H_
#include <stdlib.h>

#include "network.h"
#include "peers.h"

typedef struct {
    int udp4;
    int udp6;
    CurrentSendBehaviours* send_behaviours;
    char* user_identifier;
    Peer* peers;
    size_t peers_size;
} NetworkContext;

typedef enum {
    CMD_RETURN_ERROR = -1,
    CMD_RETURN_UNKNOWN,
    CMD_RETURN_EXIT,
    CMD_RETURN_OK,
} CmdReturnSignal;

typedef CmdReturnSignal (*CmdProcessFunction)(const char* args_string, NetworkContext* net_context);
typedef struct {
    const char* cmd_string;
    const char* cmd_help_string;
    CmdProcessFunction process_function;
} CmdProcessEntry;

/**
 * @brief Determines inputted command and forwards it to appriopriate processing function
 * @param args_string String with user input, null-terminated.
 * @param net_context Pointer to NetworkContext.
 * @return Return signal from command processing.
 */
CmdReturnSignal HandleCommand(char* input, NetworkContext* net_context);

/**
 * @brief Prints help information.
 * @param args_string String with user input, null-terminated. This command will not use this argument.
 * @param net_context Pointer to NetworkContext. This command will not use this argument.
 * @return Will always return CMD_RETURN_OK
 */
CmdReturnSignal ProcessCmdHelp(const char* args_string, NetworkContext* net_context);

/**
 * @brief Prints help information.
 * @param args_string String with user input, null-terminated. This command will not use this argument.
 * @param net_context Pointer to NetworkContext. This command will not use this argument.
 * @return Will always return CMD_RETURN_EXIT
 */
CmdReturnSignal ProcessCmdExit(const char* args_string, NetworkContext* net_context);

/**
 * @brief For unimplemented commands. Will notify user that the command is not implemeented and do nothing.
 * @param args_string String with user input, null-terminated. This command will not use this argument.
 * @param net_context Pointer to NetworkContext. This command will not use this argument.
 * @return Will always return CMD_RETURN_OK
 */
CmdReturnSignal ProcessCmdNotImplemented(const char* args_string, NetworkContext* net_context);

/**
 * @brief Prints user identifier.
 * @param args_string String with user input, null-terminated. This command will not use this argument.
 * @param net_context Pointer to NetworkContext.
 * @return Will always return CMD_RETURN_OK
 */
CmdReturnSignal ProcessCmdWhoami(const char* args_string, NetworkContext* net_context);

/**
 * @brief Prints the details of all peers.
 * @param args_string String with user input, null-terminated. This command will not use this argument.
 * @param net_context Pointer to NetworkContext.
 * @return Will always return CMD_RETURN_OK
 */
CmdReturnSignal ProcessCmdPrintPeers(const char* args_string, NetworkContext* net_context);

#endif  // SRC_COMMAND_PROCESSOR_H_

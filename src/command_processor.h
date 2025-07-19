// Copyright 2025 Michał Jankowski
#ifndef SRC_COMMAND_PROCESSOR_H_
#define SRC_COMMAND_PROCESSOR_H_

#include <stdlib.h>

typedef enum {
    CMD_RETURN_EXIT,
    CMD_RETURN_UNKNOWN,
    CMD_RETURN_OK,
} CmdReturnSignal;

typedef CmdReturnSignal (*CmdProcessFunction)(const char* args_string);
typedef struct {
    const char* cmd_string;
    const char* cmd_help_string;
    CmdProcessFunction process_function;
} CmdProcessEntry;

/**
 * @brief Determines inputted command and forwards it to appriopriate processing function
 * @param args_string String with user input, null-terminated.
 * @return Return signal from command processing.
 */
CmdReturnSignal HandleCommand(char* input);

/**
 * @brief Prints help information.
 * @param args_string String with user input, null-terminated. This command will not use this argument.
 * @return Will always return CMD_RETURN_OK
 */
CmdReturnSignal ProcessCmdHelp(const char* args_string);

/**
 * @brief Prints help information.
 * @param args_string String with user input, null-terminated. This command will not use this argument.
 * @return Will always return CMD_RETURN_EXIT
 */
CmdReturnSignal ProcessCmdExit(const char* args_string);

#endif  // SRC_COMMAND_PROCESSOR_H_

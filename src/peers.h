// Copyright 2025 Michał Jankowski
#ifndef SRC_PEERS_H_
#define SRC_PEERS_H_

#include <stdlib.h>

extern const size_t USER_IDENTIFIER_SIZE;

/**
 * @brief Creates a local user identifier.
 * @param username Username chosen by the user on startup.
 * @param user_identifier Buffer to store the created user identifier in.
 * @param user_identifier_size Size of the user identifier buffer.
 * @return 0 on success, -1 on failure.
 */
int CreateLocalUserIdentifier(
    const char* username,
    char* user_identifier,
    size_t user_identifier_size
);

#endif  // SRC_PEERS_H_

// Copyright 2025 Michał Jankowski
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "peers.h"

const size_t USER_IDENTIFIER_SIZE = 320;
const size_t HOSTNAME_SIZE = 256;
const size_t MAX_USERNAME_LENGTH = USER_IDENTIFIER_SIZE - HOSTNAME_SIZE - 2;  // -2 for '@' and null terminator

int CreateLocalUserIdentifier(
    const char* username,
    char* user_identifier,
    size_t user_identifier_size
) {
    char hostname[HOSTNAME_SIZE];

    if (username == NULL || user_identifier == NULL || user_identifier_size < USER_IDENTIFIER_SIZE) {
        return -1;
    }

    memset(user_identifier, 0, user_identifier_size);
    size_t username_length = strlen(username);
    if (username_length > MAX_USERNAME_LENGTH) {
        fprintf(
            stderr,
            "[FAIL] Username cannot be longer than %zu bytes. Current length: %zu bytes.\n",
            MAX_USERNAME_LENGTH,
            username_length);
        return -1;
    }

    if (gethostname(hostname, sizeof(hostname)) < 0) {
        perror("[FAIL] Could not get hostname");
        return -1;
    }

    snprintf(user_identifier, user_identifier_size, "%s@%s", username, hostname);
    return 0;
}

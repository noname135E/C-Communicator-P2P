// Copyright 2025 Michał Jankowski
#include <arpa/inet.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "peers.h"

const size_t USER_IDENTIFIER_SIZE = 320;
const size_t HOSTNAME_SIZE = 256;
const size_t MAX_USERNAME_LENGTH = USER_IDENTIFIER_SIZE - HOSTNAME_SIZE - 2;  // -2 for '@' and null terminator
const size_t MAX_PEERS = 32;

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

void PrintHumanReadableTime(time_t time) {
    struct tm* tm_info = localtime(&time);
    if (tm_info == NULL) {
        perror("[FAIL] Could not convert time to local time");
        return;
    }
    const size_t TIME_BUFFER_SIZE = 32;
    char buffer[TIME_BUFFER_SIZE];
    // Format: YYYY-MM-DD HH:MM:SS
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);
    printf("%s\n", buffer);
}

void PrintPeers(
    const Peer* peers,
    size_t peers_size
) {
    printf("\n");
    bool none_seen = true;
    for (size_t i = 0; i < peers_size; i++) {
        const Peer* p = &peers[i];
        if (p->ipv4.last_seen == 0 && p->ipv6.last_seen == 0) {
            continue;
        }
        none_seen = false;
        printf("Peer %zu:\n", i);
        printf("\tUser Identifier: %s\n", p->user_identifier);
        if (p->ipv4.last_seen != 0) {
            char ipv4_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &p->ipv4.addr4.sin_addr, ipv4_str, sizeof(ipv4_str));
            printf("\tIPv4 Address: %s (last seen: ", ipv4_str);
            PrintHumanReadableTime(p->ipv4.last_seen);
            printf(")\n");
        }

        if (p->ipv6.last_seen != 0) {
            char ipv6_str[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, &p->ipv6.addr6.sin6_addr, ipv6_str, sizeof(ipv6_str));
            printf("\tIPv6 Address: %s (last seen: ", ipv6_str);
            PrintHumanReadableTime(p->ipv6.last_seen);
            printf(")\n");
        }

        printf("\n");
    }

    if (none_seen) {
        printf("No peers detected yet.\n");
    }
}

ssize_t FindPeerByIpv4(
    Peer* peers,
    size_t peers_size,
    struct sockaddr_in* addr4
) {
    for (size_t i = 0; i < peers_size; i++) {
        if (memcmp(&peers[i].ipv4.addr4, addr4, sizeof(struct sockaddr_in)) == 0) {
            return i;
        }
    }
    return -1;
}

ssize_t FindPeerByIpv6(
    Peer* peers,
    size_t peers_size,
    struct sockaddr_in6* addr6
) {
    for (size_t i = 0; i < peers_size; i++) {
        if (memcmp(&peers[i].ipv6.addr6, addr6, sizeof(struct sockaddr_in6)) == 0) {
            return i;
        }
    }
    return -1;
}

ssize_t FindPeerByUserIdentifier(
    Peer* peers,
    size_t peers_size,
    const char* user_identifier
) {
    if (user_identifier == NULL) {
        return -1;
    }

    for (size_t i = 0; i < peers_size; i++) {
        if (peers[i].user_identifier != NULL &&
            strcmp(peers[i].user_identifier, user_identifier) == 0) {
            return i;
        }
    }
    return -1;
}

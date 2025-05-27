// Copyright 2025 Michał Jankowski

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>

#include "peer.h"

int SetPeerInet4(Peer peers[], const size_t peers_size, struct in_addr *addr4, const char *user_identifier) {
    int pos_by_ui = -1;
    int pos_by_addr = -1;
    int first_free = -1;

    if (addr4 == NULL || user_identifier == NULL) {
        return -1;
    }

    for (size_t i = 0; i < peers_size; i++) {
        if (peers[i].user_identifier[0] == '\0') {
            if (first_free == -1) {
                first_free = i;
            }
        } else {
                if (strncmp(peers[i].user_identifier, user_identifier, sizeof(peers[i].user_identifier)) == 0) {
                    pos_by_ui = i;
                }
                if (memcmp(&peers[i].inet4.addr4, addr4, sizeof(struct in_addr)) == 0) {
                    pos_by_addr = i;
                }
        }
    }

    if (pos_by_ui == -1) {
        if (pos_by_addr != -1) {
            RemovePeerAddressAtPosition(peers, peers_size, pos_by_addr, 1, 0);
        }
        return CreatePeerAtPosition(peers, peers_size, first_free, addr4, NULL, user_identifier);
    } else {
        if (pos_by_addr == pos_by_ui) {
            peers[pos_by_addr].inet4.seen = time(NULL);
        } else {
            if (pos_by_addr != -1) {
                RemovePeerAddressAtPosition(peers, peers_size, pos_by_addr, 1, 0);
            }
            peers[pos_by_ui].inet4.seen = time(NULL);
            memcpy(&peers[pos_by_ui].inet4.addr4, addr4, sizeof(struct in_addr));
        }
    }
    return 0;
}

int SetPeerInet6(Peer peers[], const size_t peers_size, struct in6_addr *addr6, const char *user_identifier) {
    int pos_by_ui = -1;
    int pos_by_addr = -1;
    int first_free = -1;

    if (addr6 == NULL || user_identifier == NULL) {
        return -1;
    }

    for (size_t i = 0; i < peers_size; i++) {
        if (peers[i].user_identifier[0] == '\0') {
            if (first_free == -1) {
                first_free = i;
            }
        } else {
                if (strncmp(peers[i].user_identifier, user_identifier, sizeof(peers[i].user_identifier)) == 0) {
                    pos_by_ui = i;
                }
                if (memcmp(&peers[i].inet6.addr6, addr6, sizeof(struct in6_addr)) == 0) {
                    pos_by_addr = i;
                }
        }
    }

    if (pos_by_ui == -1) {
        if (pos_by_addr != -1) {
            RemovePeerAddressAtPosition(peers, peers_size, pos_by_addr, 0, 1);
        }
        return CreatePeerAtPosition(peers, peers_size, first_free, NULL, addr6, user_identifier);
    } else {
        if (pos_by_addr == pos_by_ui) {
            peers[pos_by_addr].inet6.seen = time(NULL);
        } else {
            if (pos_by_addr != -1) {
                RemovePeerAddressAtPosition(peers, peers_size, pos_by_addr, 0, 1);
            }
            peers[pos_by_ui].inet6.seen = time(NULL);
            memcpy(&peers[pos_by_ui].inet6.addr6, addr6, sizeof(struct in6_addr));
        }
    }
    return 0;
}

int CreatePeerAtPosition(
    Peer peers[],
    const size_t peers_size,
    int pos,
    struct in_addr *addr4,
    struct in6_addr *addr6,
    const char *user_identifier) {
    if (pos < 0) {
        return -1;
    }
    size_t actual_position = (size_t) pos;
    if (actual_position >= peers_size || peers == NULL || user_identifier == NULL) {
        return -1;
    }

    time_t now = time(NULL);

    Peer *p = &peers[actual_position];

    strncpy(p->user_identifier, user_identifier, sizeof(p->user_identifier) - 1);
    p->user_identifier[sizeof(p->user_identifier) - 1] = '\0';

    short remove_ipv4 = 0;
    short remove_ipv6 = 0;

    if (addr4 != NULL) {
        p->inet4.addr4 = *addr4;
        p->inet4.seen = now;
    } else {
        remove_ipv4 = 1;
    }

    if (addr6 != NULL) {
        p->inet6.addr6 = *addr6;
        p->inet6.seen = now;
    } else {
        remove_ipv6 = 1;
    }

    if (remove_ipv4 + remove_ipv6 != 0) {
        return RemovePeerAddressAtPosition(peers, peers_size, actual_position, remove_ipv4, remove_ipv6);
    }
    return 0;
}

int RemovePeerAddressAtPosition(
    Peer peers[],
    const size_t peers_size,
    size_t pos,
    short remove_ipv4,
    short remove_ipv6) {
    if (!peers || pos >= peers_size || (remove_ipv4 == 0 && remove_ipv6 == 0)) {
        return -1;
    }

    Peer *p = &peers[pos];
    if (remove_ipv4) {
        memset(&p->inet4, 0, sizeof(p->inet4));
    }
    if (remove_ipv6) {
        memset(&p->inet6, 0, sizeof(p->inet6));
    }
    if (p->inet4.seen == 0 && p->inet6.seen == 0) {
        memset(p, 0, sizeof(Peer));
    }
    return 0;
}

void PrintPeers(Peer peers[], const size_t peers_size) {
    short none_seen = 1;
    for (size_t i = 0; i < peers_size; ++i) {
        Peer *p = &peers[i];

        if (p->inet4.seen == 0 && p->inet6.seen == 0) {
            continue;
        }
        none_seen = 0;

        printf("Peer %zu:\n", i);
        printf("  User Identifier: %s\n", p->user_identifier);

        if (p->inet4.seen != 0) {
            char ipv4_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &p->inet4.addr4, ipv4_str, sizeof(ipv4_str));
            printf("  IPv4 Address: %s (seen: ", ipv4_str);
            PrintHumanReadableTime(p->inet4.seen);
            printf(")\n");
        }

        if (p->inet6.seen != 0) {
            char ipv6_str[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, &p->inet6.addr6, ipv6_str, sizeof(ipv6_str));
            printf("  IPv6 Address: %s (seen: ", ipv6_str);
            PrintHumanReadableTime(p->inet6.seen);
            printf(")\n");
        }

        printf("\n");
    }
    if (none_seen) {
        printf("No peers detected yet.\n");
    }
}

void PrintHumanReadableTime(time_t t) {
    char buffer[64];
    struct tm *tm_info = localtime(&t);
    if (tm_info == NULL) {
        printf("Invalid time\n");
        return;
    }

    // Format: YYYY-MM-DD HH:MM:SS
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);
    printf("%s", buffer);
}

void ClearAllPeers(Peer peers[], const size_t peers_size) {
    if (peers == NULL || peers_size == 0) {
        return;
    }
    memset(peers, 0, sizeof(Peer) * peers_size);
}

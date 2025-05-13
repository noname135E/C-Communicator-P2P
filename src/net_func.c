// Copyright 2025 Michał Jankowski
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "net_func.h"
#include "sock_prep.h"

int send_scan(int udp4, int udp6, int ifindex) {
    const struct sockaddr *dest_addr;
    const char *msg = "Hello, world!";  // TODO(.): Change to actual text later
    const unsigned int msg_length = strlen(msg);  // if null terminator must be sent remember to add +1

    if (udp4 >= 0) {
        struct sockaddr_in ipv4_addr;
        memset(&ipv4_addr, 0, sizeof(ipv4_addr));
        ipv4_addr.sin_family = AF_INET;
        ipv4_addr.sin_port = htons(PORT);
        if (inet_pton(AF_INET, MCAST_GROUP, &ipv4_addr.sin_addr) <= 0) {
            fprintf(stderr, "[FAIL] Scan failed during IPv4 address preparation\n");
            exit(-1);
        }

        dest_addr = (const struct sockaddr *)&ipv4_addr;
        if (sendto(udp4, msg, msg_length, 0, dest_addr, sizeof(ipv4_addr)) < 0) {
            perror("[WARN] Scan failed for IPv4");
        }
    }

    if (udp6 >= 0) {
        struct sockaddr_in6 ipv6_addr;
        memset(&ipv6_addr, 0, sizeof(ipv6_addr));
        ipv6_addr.sin6_family = AF_INET6;
        ipv6_addr.sin6_port = htons(PORT);
        ipv6_addr.sin6_scope_id = ifindex;
        if (inet_pton(AF_INET6, MCAST6_GROUP, &ipv6_addr.sin6_addr) <= 0) {
            fprintf(stderr, "[FAIL] Scan failed during IPv6 address preparation\n");
            exit(-1);
        }

        dest_addr = (const struct sockaddr *)&ipv6_addr;
        if (sendto(udp6, msg, msg_length, 0, dest_addr, sizeof(ipv6_addr)) < 0) {
            perror("[WARN] Scan failed for IPv6");
        }
    }

    return 0;
}

// Copyright 2025 Michał Jankowski
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "net_func.h"
#include "peer.h"
#include "sock_prep.h"

long int Encapsulate(const uint8_t msg_type, char *msg, const size_t buf_size) {
    uint16_t crc12, prefix;
    size_t msg_length = strlen(msg);
    if (msg_type > 15) {
        return -1;
    } else if (msg_length + 2 > buf_size) {
        return -2;
    }
    crc12 = 0;  // TODO(.): Have this be calculated later
    prefix = (crc12 << 4) | msg_type;
    memmove(msg + 2, msg, msg_length + 1);
    msg[0] = (prefix >> 8) & 0xFF;
    msg[1] = prefix & 0xFF;
    return msg_length + 2;
}

int Deencapsulate(char *msg) {
    size_t msg_length = strlen(msg);
    if (msg_length < 2) {
        return -1;
    }
    uint16_t prefix = ((uint8_t)msg[0] << 8) | (uint8_t)msg[1];
    uint8_t msg_type = prefix & 0x0F;
    uint16_t crc12 = prefix >> 4;

    // TODO(.): Check CRC

    memmove(msg, msg + 2, msg_length);
    return msg_type;
}

void ListenUDP(int udp, Peer peers[], const size_t peers_size) {
    return;
}

int SendScan(int udp4, int udp6, int ifindex, const char* user_identifier) {
    const struct sockaddr *dest_addr;
    size_t msg_length;

    char msg[512];
    if (strlen(user_identifier) > sizeof(msg)) {
        fprintf(stderr, "[FAIL] Scan attempted to send excessively long user identifier\n");
        exit(-1);
    }
    snprintf(msg, sizeof(msg), "%s", user_identifier);

    long int encap_length;
    if ((encap_length = Encapsulate(0, msg, sizeof(msg))) < 0) {
        fprintf(stderr, "[FAIL] Scan: failed to encapsulate message, error %li\n", encap_length);
        return -1;
    }
    msg_length = (size_t) encap_length;

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

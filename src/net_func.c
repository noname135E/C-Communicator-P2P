// Copyright 2025 Michał Jankowski
#include <arpa/inet.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "net_func.h"
#include "peer.h"
#include "sock_prep.h"

long int Encapsulate(const enum MessageType msg_type, char *msg, const size_t buf_size) {
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

int Deencapsulate(char *msg, ssize_t msg_length) {
    if (msg_length < 2) {
        return -1;
    }

    uint16_t prefix = ((uint8_t)msg[0] << 8) | (uint8_t)msg[1];
    uint8_t msg_type = prefix & 0x0F;
    uint16_t crc12 = prefix >> 4;

    // TODO(.): Check CRC

    memmove(msg, msg + 2, msg_length - 2);
    msg[msg_length - 2] = '\0';
    return ((int) msg_type);
}

void ListenUDP(int udp, Peer peers[], const size_t peers_size, const char* user_identifier) {
    size_t BUFFER_SIZE = 2048;
    char buffer[BUFFER_SIZE];
    struct sockaddr_storage src_addr;
    socklen_t src_addr_size = sizeof(src_addr);

    ssize_t recv_length = recvfrom(udp, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr*) &src_addr, &src_addr_size);
    if (recv_length < 0) {
        return;
    }
    buffer[recv_length] = '\0';
    int msg_type = Deencapsulate(buffer, recv_length);
    size_t msg_length = strlen(buffer);

    // TODO(.): Remove thi later, debug only
    printf("Received message of type: %i\n", msg_type);
    printf("Contents: %s\n", buffer);

    switch (msg_type) {
        case SCAN:
            // send SCAN_RESPONSE and add to peers
            ProcessMessageScan(
                udp,
                user_identifier,
                peers,
                peers_size,
                buffer,
                msg_length,
                &src_addr,
                src_addr_size);
            break;
        case SCAN_RESPONSE:
            ProcessMessageScanResponse(
                peers,
                peers_size,
                buffer,
                msg_length,
                &src_addr);
            break;
        default:
            return;
    }
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
    const enum MessageType msg_type = SCAN;
    if ((encap_length = Encapsulate(msg_type, msg, sizeof(msg))) < 0) {
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

int SendScanResponse(int udp, const char *user_identifier, struct sockaddr_storage* src_addr, socklen_t src_addr_size) {
    char msg[512];
    size_t msg_length;

    if (strlen(user_identifier) > sizeof(msg)) {
        fprintf(stderr, "[FAIL] Scan Response attempted to send excessively long user identifier\n");
        exit(-1);
    }
    snprintf(msg, sizeof(msg), "%s", user_identifier);

    long int encap_length;
    const enum MessageType msg_type = SCAN_RESPONSE;
    if ((encap_length = Encapsulate(msg_type, msg, sizeof(msg))) < 0) {
        fprintf(stderr, "[FAIL] Scan Response: failed to encapsulate message, error %li\n", encap_length);
        return -1;
    }
    msg_length = (size_t) encap_length;

    int bytes_sent;
    bytes_sent = sendto(udp, msg, msg_length, 0, (struct sockaddr*) src_addr, src_addr_size);
    return (bytes_sent < 0) ? -1 : 0;
}

void ProcessMessageScan(
    int udp,
    const char* user_identifier,
    Peer peers[],
    const size_t peers_size,
    char* msg,
    size_t msg_length,
    struct sockaddr_storage* src_addr,
    socklen_t src_addr_size
) {
    SendScanResponse(
        udp,
        user_identifier,
        src_addr,
        src_addr_size);

    char src_user_identifier[320];
    size_t copy_len = msg_length < 320 ? msg_length : 320;
    memcpy(src_user_identifier, msg, copy_len);
    src_user_identifier[copy_len] = '\0';
    if (src_addr->ss_family == AF_INET) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)src_addr;
        SetPeerInet4(peers, peers_size, &addr4->sin_addr, src_user_identifier);
    } else if (src_addr->ss_family == AF_INET6) {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)src_addr;
        SetPeerInet6(peers, peers_size, &addr6->sin6_addr, src_user_identifier);
    }
    return;
}

void ProcessMessageScanResponse(
    Peer peers[],
    const size_t peers_size,
    char* msg,
    size_t msg_length,
    struct sockaddr_storage* src_addr
) {
    char src_user_identifier[320];
    size_t copy_len = msg_length < 320 ? msg_length : 320;
    memcpy(src_user_identifier, msg, copy_len);
    src_user_identifier[copy_len] = '\0';
    if (src_addr->ss_family == AF_INET) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)src_addr;
        SetPeerInet4(peers, peers_size, &addr4->sin_addr, src_user_identifier);
    } else if (src_addr->ss_family == AF_INET6) {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)src_addr;
        SetPeerInet6(peers, peers_size, &addr6->sin6_addr, src_user_identifier);
    }
    return;
}

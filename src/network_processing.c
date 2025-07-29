// Copyright 2025 Michał Jankowski
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "network.h"
#include "network_processing.h"

SendStatus ScanLocalMulticast(NetworkContext* net_context, bool print_errors) {
    if (net_context == NULL) {
        fprintf(stderr, "[ ERR] ScanLocalMulticast: received NULL pointer.\n");
        return SEND_IPV4_ERR_OTHER | SEND_IPV6_ERR_OTHER;
    }

    size_t user_identifier_length = strlen(net_context->user_identifier);
    if (net_context->user_identifier == NULL ||
        user_identifier_length == 0 ||
        user_identifier_length >= USER_IDENTIFIER_SIZE) {
        fprintf(stderr, "[ ERR] ScanLocalMulticast: invalid user_identifier.\n");
        return SEND_IPV4_ERR_OTHER | SEND_IPV6_ERR_OTHER;
    }

    SendStatus status = 0;
    struct sockaddr_in addr4;
    memset(&addr4, 0, sizeof(addr4));
    addr4.sin_family = AF_INET;
    addr4.sin_port = htons(PROGRAM_PORT);
    if (inet_pton(AF_INET, MCAST4_GROUP, &addr4.sin_addr) != 1) {
        if (print_errors) fprintf(stderr, "[ ERR] Invalid IPv4 multicast address.\n");
        status |= SEND_IPV4_ERR_INVALID_ADDR;
    }

    struct sockaddr_in6 addr6;
    memset(&addr6, 0, sizeof(addr6));
    addr6.sin6_family = AF_INET6;
    addr6.sin6_port = htons(PROGRAM_PORT);
    addr6.sin6_scope_id = net_context->ifindex;
    if (inet_pton(AF_INET6, MCAST6_GROUP, &addr6.sin6_addr) != 1) {
        if (print_errors) fprintf(stderr, "[ ERR] Invalid IPv6 multicast address.\n");
        status |= SEND_IPV6_ERR_INVALID_ADDR;
    }

    if (status != 0) {
        return status;
    }

    return SendUDP(
        MSG_TYPE_SCAN,
        net_context->user_identifier,
        net_context->udp4,
        &addr4,
        net_context->udp6,
        &addr6,
        net_context->send_behaviours->both,
        print_errors);
}

void HandleIncomingUDP(NetworkContext* net_context, int udp_fd) {
    if (net_context == NULL) {
        fprintf(stderr, "[ ERR] HandleIncomingUDP: received NULL pointer.\n");
        return;
    } else if (udp_fd < 0) {
        fprintf(stderr, "[ ERR] HandleIncomingUDP: invalid file descriptor.\n");
        return;
    }

    ReceivedMessage received_msg;
    memset(&received_msg, 0, sizeof(received_msg));
    if (ReceiveUDP(&received_msg, udp_fd) == 0) {
        return;  // message invalid
    }

    switch (received_msg.msg_type) {
        case MSG_TYPE_SCAN:
            // TODO(.): Function to handle responding
            break;
        case MSG_TYPE_SCAN_RESPONSE:
            // TODO(.): Function to handle scan responses
            break;
        default:
            fprintf(
                stderr,
                "[ ERR] HandleIncomingUDP: message type %d, which could not be processed.\n",
                received_msg.msg_type);
    }
}

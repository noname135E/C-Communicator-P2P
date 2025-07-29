// Copyright 2025 Michał Jankowski
#ifndef SRC_NETWORK_PROCESSING_H_
#define SRC_NETWORK_PROCESSING_H_
#include <stdbool.h>

#include "network.h"
#include "network_context.h"

/**
 * @brief Scans the local network for peers by sending multicast messages.
 * @param net_context Pointer to NetworkContext containing network configuration.
 * @param print_errors If true, prints error messages to stderr.
 * @return SendStatus indicating the result of the scan operation.
 */
SendStatus ScanLocalMulticast(
    NetworkContext* net_context,
    bool print_errors
);

/**
 * @brief Handles incoming UDP messages of type MSG_TYPE_SCAN.
 * @param net_context Pointer to NetworkContext containing network configuration.
 * @param received_msg Pointer to ReceivedMessage struct containing the received message.
 */
void HandleIncomingScan(
    NetworkContext* net_context,
    ReceivedMessage* received_msg
);

/**
 * @brief Handles incoming UDP messages of type MSG_TYPE_SCAN_RESPONSE.
 * @param net_context Pointer to NetworkContext containing network configuration.
 * @param received_msg Pointer to ReceivedMessage struct containing the received message.
 */
void HandleIncomingScanResponse(
    NetworkContext* net_context,
    ReceivedMessage* received_msg
);

/**
 * @brief Processes incoming UDP messages and handles them accordingly.
 * @param net_context Pointer to NetworkContext containing network configuration.
 * @param udp_fd File descriptor for the UDP socket to handle from.
 */
void HandleIncomingUDP(
    NetworkContext* net_context,
    int udp_fd
);

#endif  // SRC_NETWORK_PROCESSING_H_

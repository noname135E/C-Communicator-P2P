// Copyright 2025 Michał Jankowski
#ifndef SRC_NETWORK_H_
#define SRC_NETWORK_H_

#include <netinet/in.h>

#include "common.h"


// using BOTH implies that both sockets must work
typedef enum {
    SEND_IPV4_ONLY,
    SEND_IPV6_ONLY,
    SEND_IPV4_FIRST,
    SEND_IPV6_FIRST,
    SEND_BOTH,
} SendBehaviour;

typedef enum {
    CAST_TYPE_NULL,
    CAST_TYPE_INVALID,
    CAST_TYPE_UNICAST,
    CAST_TYPE_MULTICAST,
    // broadcast intentionally skipped - no usage planned.
} IpAddrCastType;

typedef enum {
    SCOPE_TYPE_NULL,
    SCOPE_TYPE_INVALID_LINKLOCAL,  // for address indicating link-local but scope id of 0
    SCOPE_TYPE_LINKLOCAL,
    SCOPE_TYPE_GLOBAL,
} Ipv6ScopeType;

typedef enum {
    SEND_IPV4_OK = 0x00,
    SEND_IPV4_NOT_ATTEMPTED = 0x01,
    SEND_IPV4_ERR_INVALID_FD = 0x02,
    SEND_IPV4_ERR_INVALID_ADDR = 0x03,
    SEND_IPV4_ERR_INVALID_MSG_LEN = 0x04,
    SEND_IPV4_ERR_INVALID_MSG_PTR = 0x05,
    SEND_IPV4_ERR_PARTIALLY_SENT = 0x06,
    SEND_IPV4_ERR_CAST_MISMATCH = 0x07,
    SEND_IPV4_ERR_ENCAPSULATION = 0x08,
    // 0x0E should represent an IPv4-only error or be skipped
    SEND_IPV4_ERR_OTHER = 0x0F,
    SEND_IPV4_MASK = 0x0F,

    SEND_IPV6_OK = 0x00,
    SEND_IPV6_NOT_ATTEMPTED = 0x10,
    SEND_IPV6_ERR_INVALID_FD = 0x20,
    SEND_IPV6_ERR_INVALID_ADDR = 0x30,
    SEND_IPV6_ERR_INVALID_MSG_LEN = 0x40,
    SEND_IPV6_ERR_INVALID_MSG_PTR = 0x50,
    SEND_IPV6_ERR_PARTIALLY_SENT = 0x60,
    SEND_IPV6_ERR_CAST_MISMATCH = 0x70,
    SEND_IPV6_ERR_ENCAPSULATION = 0x80,
    SEND_IPV6_ERR_INVALID_SCOPE = 0xE0,
    SEND_IPV6_ERR_OTHER = 0xF0,
    SEND_IPV6_MASK = 0xF0,
} SendStatus;

/**
 * @brief Calculates CRC-12 checksum for string in buf.
 * @param buf String to calculate checksum for.
 * @param buf_len Length of string - required, as buf is not expected to be null-terminated.
 * @return Calculated checksum
 */
uint16_t CalculateChecksum(char* buf, size_t buf_len);

/**
 * @brief Encapsulates message in the correct format.
 * @param msg_type Type of message.
 * @param msg_to_encapsulate Null-terminated string containing message to be sent.
 * @param encapsulated_binary Buffer to put encapsulated message in. NOT NULL-TERMINATED
 * @param encapsulated_size Size of encapsulated_binary.
 *                          Should be no less than 1 byte larger than msg_to_encapsulate (or 2 bytes larger than length).
 * @return Length of encapsulated_binary. 0 indicates failure.
 */
size_t Encapsulate(
    const MessageType msg_type,
    char* msg_to_encapsulate,
    char* encapsulated_binary,
    const size_t encapsulated_size
);

/**
 * @brief Deencapsulates message to null-terminated string and MessageType.
 * @param encapsulated_binary Buffer containing encapsulated message. NOT NULL TERMINATED.
 * @param encapsulated_size Size of encapsulated_binary.
 * @param deencapsulated_msg Buffer to put null-terminated deencapsulated message in.
 * @param msg_size Size of deencapsulated_msg. Size equal to encapsulated_size - 1 should be sufficient.
 * @return Type of message. If unable to deencapsulate, will return MSG_INVALID.
 */
MessageType Deencapsulate(
    char* encapsulated_binary,
    const size_t encapsulated_size,
    char* deencapsulated_msg,
    const size_t msg_size
);

IpAddrCastType GetIPv4CastType(struct sockaddr_in* addr4);

IpAddrCastType GetIPv6CastType(struct sockaddr_in6* addr6);

Ipv6ScopeType GetIPv6ScopeType(struct sockaddr_in6* addr6);

SendStatus HelperIPv4SendUDP(
    char* msg,
    const size_t msg_size,
    int udp4,
    struct sockaddr_in* addr4,
    short print_errors
);

SendStatus HelperIPv6SendUDP(
    char* msg,
    const size_t msg_size,
    int udp6,
    struct sockaddr_in6* addr6,
    short print_errors
);

SendStatus SendUDP(
    const MessageType msg_type,
    char* msg,
    int udp4,
    struct sockaddr_in* addr4,
    int udp6,
    struct sockaddr_in6* addr6,
    const SendBehaviour behaviour,
    short print_errors
);

int ReceiveUDP();
// TODO(.): Figure out what exactly I want this to do"
// - just receive and deencapsulate?
// - also call process?

#endif  // SRC_NETWORK_H_

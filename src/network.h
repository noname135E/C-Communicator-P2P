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

/**
 * @brief Identifies whether IPv4 address is unicast or multicast
 * @param addr4 sockaddr_in containing IPv4 to check
 * @return Determined IpAddrCastType
 */
IpAddrCastType GetIPv4CastType(struct sockaddr_in* addr4);

/**
 * @brief Identifies whether IPv6 address is unicast or multicast
 * @param addr6 sockaddr_in6 containing IPv6 to check
 * @return Determined IpAddrCastType
 */
IpAddrCastType GetIPv6CastType(struct sockaddr_in6* addr6);

/**
 * @brief Identifies whether IPv6 address is Link-Local and contains a scope_id or Global
 * @param addr6 sockaddr_in6 containing IPv6 to check
 * @return Determined Ipv6ScopeType
 */
Ipv6ScopeType GetIPv6ScopeType(struct sockaddr_in6* addr6);

/**
 * @brief Helper function for SendUDP for handling IPv4
 * @param msg Buffer containing message to send. NOT NULL TERMINATED.
 * @param msg_size Size of msg to send.
 * @param udp4 File descriptor of IPv4/UDP socket. If < 0 SEND_IPV4_ERR_INVALID_FD will be returned.
 * @param addr4 Destination sockaddr_in struct
 * @param print_errors 1 if errors should be printed, else 0.
 * @return SendStatus of the sending.
 */
SendStatus HelperIPv4SendUDP(
    char* msg,
    const size_t msg_size,
    int udp4,
    struct sockaddr_in* addr4,
    short print_errors
);

/**
 * @brief Helper function for SendUDP for handling IPv6
 * @param msg Buffer containing message to send. NOT NULL TERMINATED.
 * @param msg_size Size of msg to send.
 * @param udp6 File descriptor of IPv6/UDP socket. If < 0 SEND_IPV6_ERR_INVALID_FD will be returned.
 * @param addr6 Destination sockaddr_in6 struct
 * @param print_errors 1 if errors should be printed, else 0.
 * @return SendStatus of the sending.
 */
SendStatus HelperIPv6SendUDP(
    char* msg,
    const size_t msg_size,
    int udp6,
    struct sockaddr_in6* addr6,
    short print_errors
);

/**
 * @brief Function to send UDP messages capable of handling IPv4 and IPv6, including fallback.
 * Messages are encapsulated by this function as part of the process.
 * @param msg_type MessageType of the message to be sent.
 * @param msg Buffer containing null-terminated contents to send.
 * @param udp4 File descriptor of IPv4/UDP socket. If set to negative value, socket will be treated as unavailable.
 * @param addr4 Destination sockaddr_in for IPv4. Can be null if not needed.
 * @param udp6 File descriptor of IPv6/UDP socket. If set to negative value, socket will be treated as unavailable.
 * @param addr6 Destination sockaddr_in6 for IPv6. Can be null if not needed. For link-local addresses scope_id must be set!
 * @param behaviour How the function should achieve sending of the message. If behaviour permits/calls for use of unavailable socket
 * function will interpret the attempt as failed due to invalid fd.
 * @param print_errors 1 if errors should be printed, else 0.
 * @return Combined SendStatus of IPv4 and IPv6 sending attempts. Beware: Even if behaviour calls for only one of them, the other will
 * have a status of not attempted.
 */
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

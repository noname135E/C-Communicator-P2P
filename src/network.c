// Copyright 2025 Michał Jankowski
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "network.h"

const size_t HEADER_LENGTH = 2;
const size_t MAX_UDP_PAYLOAD_SIZE = 65487;  // IPv4's is slightly larger
const size_t MAX_UDP_MESSAGE_SIZE = 4096;  // feels sufficiently large

uint16_t CalculateChecksum(char* buf, size_t buf_len) {
    const uint16_t poly = 0x80F;
    uint16_t checksum = 0x000;

    for (size_t byte = 0; byte < buf_len; ++byte) {
        checksum ^= (uint16_t) (buf[byte] << 4);
        for (int bit = 0; bit < 8; ++bit) {
            if (checksum & 0x800) {
                checksum = (checksum << 1) ^ poly;
            } else {
                checksum <<= 1;
            }
            checksum &= 0xFFF;
        }
    }

    return checksum;
}

size_t Encapsulate(
    const MessageType msg_type,
    char* msg_to_encapsulate,
    char* encapsulated_binary,
    const size_t encapsulated_size
) {
    if (msg_type < 0 || msg_type > 15) return 0;

    uint16_t checksum, header;
    size_t msg_len = strlen(msg_to_encapsulate);

    if (msg_len + HEADER_LENGTH > encapsulated_size) return 0;

    memset(encapsulated_binary, 0, encapsulated_size);
    // ! Encapsulated binary does not include null terminator
    memcpy(encapsulated_binary + HEADER_LENGTH, msg_to_encapsulate, msg_len);
    encapsulated_binary[1] = msg_type & 0x0F;  // msg_type also protected
    checksum = CalculateChecksum(encapsulated_binary + 1, msg_len + 1);

    header = (checksum << 4) | msg_type;
    // Encode checksum and type in big Endian
    encapsulated_binary[0] = (header >> 8) & 0xFF;
    encapsulated_binary[1] = header & 0xFF;  // replaced by correct encoding
    return msg_len + HEADER_LENGTH;
}

MessageType Deencapsulate(
    char* encapsulated_binary,
    const size_t encapsulated_size,
    char* deencapsulated_msg,
    const size_t msg_size
) {
    const size_t msg_part_length = encapsulated_size - HEADER_LENGTH;
    memset(deencapsulated_msg, 0, msg_size);

    if (encapsulated_size < HEADER_LENGTH) return MSG_INVALID;
    // + 1 for the null-terminator
    if (msg_size < msg_part_length + 1) return MSG_INVALID;
    if (MSG_BUFFER_SIZE < msg_part_length + 1) return MSG_INVALID;

    uint16_t header = ((uint8_t)encapsulated_binary[0] << 8) | (uint8_t)encapsulated_binary[1];
    uint16_t received_checksum = header >> 4;
    uint8_t received_msg_type = header & 0x0F;

    char temp_buf[MSG_BUFFER_SIZE];
    memset(temp_buf, 0, MSG_BUFFER_SIZE);
    temp_buf[0] = received_msg_type;
    // copy over message part
    char* start_of_msg = &temp_buf[1];
    memcpy(start_of_msg, encapsulated_binary + HEADER_LENGTH, msg_part_length);

    if (received_checksum != CalculateChecksum(temp_buf, msg_part_length + 1)) return MSG_INVALID;
    memcpy(deencapsulated_msg, start_of_msg, msg_part_length);
    deencapsulated_msg[msg_part_length] = '\0';
    return (MessageType) received_msg_type;
}

SendStatus HelperIPv4SendUDP(
    char* msg,
    const size_t msg_size,
    int udp4,
    struct sockaddr_in* addr4,
    short print_errors
) {
    ssize_t bytes_sent;

    if (udp4 < 0) {
        return SEND_IPV4_ERR_INVALID_FD;
    } else if  (msg_size < HEADER_LENGTH || msg_size > MAX_UDP_PAYLOAD_SIZE) {
        return SEND_IPV4_ERR_INVALID_MSG_LEN;
    } else if (addr4 == NULL)  {
        return SEND_IPV4_ERR_INVALID_ADDR;
    } else if (msg == NULL) {
        return SEND_IPV4_ERR_INVALID_MSG_PTR;
    }

    bytes_sent = sendto(udp4, msg, msg_size, 0, (const struct sockaddr *) addr4, sizeof(*addr4));

    if (bytes_sent == msg_size) {
        return SEND_IPV4_OK;
    } else if (bytes_sent > 0) {
        return SEND_IPV4_ERR_PARTIALLY_SENT;
    }
    if (print_errors) {
        perror("[FAIL] Could not send IPv4/UDP");
    }
    switch (errno) {
        case EBADF:
        case ENOTSOCK:
            return SEND_IPV4_ERR_INVALID_FD;
            break;
        case EMSGSIZE:
            return SEND_IPV4_ERR_INVALID_MSG_LEN;
            break;
        default:
            return SEND_IPV4_ERR_OTHER;
            break;
    }
    // Error mapping likely incomplete
}

SendStatus HelperIPv6SendUnicastUDP(
    char* msg,
    const size_t msg_size,
    int udp6,
    struct sockaddr_in6* addr6,
    short print_errors
) {
    ssize_t bytes_sent;

    if (udp6 < 0) {
        return SEND_IPV6_ERR_INVALID_FD;
    } else if  (msg_size < HEADER_LENGTH || msg_size > MAX_UDP_PAYLOAD_SIZE) {
        return SEND_IPV6_ERR_INVALID_MSG_LEN;
    } else if (addr6 == NULL)  {
        return SEND_IPV6_ERR_INVALID_ADDR;
    } else if (msg == NULL) {
        return SEND_IPV6_ERR_INVALID_MSG_PTR;
    }

    bytes_sent = sendto(udp6, msg, msg_size, 0, (const struct sockaddr *) addr6, sizeof(*addr6));

    if (bytes_sent == msg_size) {
        return SEND_IPV6_OK;
    } else if (bytes_sent > 0) {
        return SEND_IPV6_ERR_PARTIALLY_SENT;
    }
    if (print_errors) {
        perror("[FAIL] Could not send IPv4/UDP");
    }
    switch (errno) {
        case EBADF:
        case ENOTSOCK:
            return SEND_IPV6_ERR_INVALID_FD;
            break;
        case EMSGSIZE:
            return SEND_IPV6_ERR_INVALID_MSG_LEN;
            break;
        default:
            return SEND_IPV6_ERR_OTHER;
            break;
    }
    // Error mapping likely incomplete
}

SendStatus SendUnicastUDP(
    const MessageType msg_type,
    char* msg,
    int udp4,
    struct sockaddr_in* addr4,
    int udp6,
    struct sockaddr_in6* addr6,
    const SendBehaviour behaviour,
    short print_errors
) {
    SendStatus status4 = SEND_IPV4_NOT_ATTEMPTED;
    SendStatus status6 = SEND_IPV6_NOT_ATTEMPTED;

    char encapsulated_binary[MAX_UDP_MESSAGE_SIZE];
    memset(encapsulated_binary, 0, MAX_UDP_MESSAGE_SIZE);

    size_t encapsulated_binary_length = Encapsulate(msg_type, msg, encapsulated_binary, MAX_UDP_MESSAGE_SIZE);

    if (encapsulated_binary_length < HEADER_LENGTH) {
        return SEND_IPV4_ERR_ENCAPSULATION || SEND_IPV6_ERR_ENCAPSULATION;
    }

    if (behaviour == SEND_IPV4_ONLY || behaviour == SEND_IPV4_FIRST || behaviour == SEND_BOTH) {
        status4 = HelperIPv4SendUDP(msg, encapsulated_binary_length, udp4, addr4, print_errors);
    }

    if (behaviour == SEND_IPV6_FIRST ||
        behaviour == SEND_IPV6_ONLY ||
        behaviour == SEND_BOTH ||
        (behaviour == SEND_IPV4_FIRST && status4 != SEND_IPV4_OK)) {
        status6 = HelperIPv6SendUnicastUDP(msg, encapsulated_binary_length, udp6, addr6, print_errors);
    }

    if (behaviour == SEND_IPV6_FIRST && status6 != SEND_IPV6_OK) {
        status4 = HelperIPv4SendUDP(msg, encapsulated_binary_length, udp4, addr4, print_errors);
    }

    return status4 || status6;
}

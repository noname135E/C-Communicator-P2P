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

IpAddrCastType GetIPv4CastType(struct sockaddr_in* addr4) {
    if (addr4 == NULL) return CAST_TYPE_NULL;

    uint32_t ip_addr = ntohl(addr4->sin_addr.s_addr);
    // (0, 224.0.0.0) = Unicast
    if (ip_addr > 0 && ip_addr < 0xE0000000) return CAST_TYPE_UNICAST;
    // [224.0.0.0, 239.255.255.255] = Multicast
    else if (ip_addr <= 0xEFFFFFFF) return CAST_TYPE_MULTICAST;
    // 0.0.0.0, 255.255.255.255 or other
    return CAST_TYPE_INVALID;
}

IpAddrCastType GetIPv6CastType(struct sockaddr_in6* addr6) {
    if (addr6 == NULL) return CAST_TYPE_NULL;

    const unsigned char *bytes = addr6->sin6_addr.s6_addr;
    // FF at the start = Multicast
    if (bytes[0] == 0xFF) return CAST_TYPE_MULTICAST;
    // ::0 address
    static const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;
    if (memcmp(&addr6->sin6_addr, &in6addr_any, sizeof(struct in6_addr)) == 0) {
        return CAST_TYPE_INVALID;
    }
    // all other addresses are either Unicast or Anycast
    return CAST_TYPE_UNICAST;
}

Ipv6ScopeType GetIPv6ScopeType(struct sockaddr_in6* addr6) {
    if (addr6 == NULL) return SCOPE_TYPE_NULL;

    const unsigned char *bytes = addr6->sin6_addr.s6_addr;

    // ::0 address
    static const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;
    if (memcmp(&addr6->sin6_addr, &in6addr_any, sizeof(struct in6_addr)) == 0) {
        return SCOPE_TYPE_NULL;
    }

    // FF at the start = Multicast
    if (bytes[0] == 0xFF) {
        uint8_t scope = bytes[1] & 0x0F;
        switch (scope) {
            // interface-local not planned to be supported anyway
            case 0x2:
                if (addr6->sin6_scope_id == 0) return SCOPE_TYPE_INVALID_LINKLOCAL;
                else return SCOPE_TYPE_LINKLOCAL;
                break;
            default:
                return SCOPE_TYPE_GLOBAL;  // As the check is important for scope id, other can bee treated as global
        }
    }

    // Link-local Unicast (fe80::)
    if ((bytes[0] == 0xFE) && ((bytes[1] & 0xC0) == 0x80)) {
        if (addr6->sin6_scope_id == 0) {
            return SCOPE_TYPE_INVALID_LINKLOCAL;
        }
        return SCOPE_TYPE_LINKLOCAL;
    }

    // All other addresses are considered global
    return SCOPE_TYPE_GLOBAL;
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

SendStatus HelperIPv6SendUDP(
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
        perror("[FAIL] Could not send IPv6/UDP");
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

SendStatus SendUDP(
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

    const IpAddrCastType ct4 = GetIPv4CastType(addr4);
    const IpAddrCastType ct6 = GetIPv6CastType(addr6);

    const Ipv6ScopeType st6 = GetIPv6ScopeType(addr6);
    if (st6 == SCOPE_TYPE_INVALID_LINKLOCAL) status6 = SEND_IPV6_ERR_INVALID_SCOPE;

    // arguably some of these checks can be skipped for certain behaviours,
    // but no good reason to add more logic, checking for mismatches already feels on the edge of excess
    if (ct4 != CAST_TYPE_NULL && ct6 != CAST_TYPE_NULL) {  // at least one not NULL

        if (ct4 == CAST_TYPE_UNICAST && ct6 == CAST_TYPE_MULTICAST ||  // mismatch
            ct4 == CAST_TYPE_MULTICAST && ct6 == CAST_TYPE_UNICAST) {
                return SEND_IPV4_ERR_CAST_MISMATCH || SEND_IPV6_ERR_CAST_MISMATCH;
            }

        if (ct4 == CAST_TYPE_INVALID) status4 = SEND_IPV4_ERR_INVALID_ADDR;
        if (ct6 == CAST_TYPE_INVALID) status6 = SEND_IPV6_ERR_INVALID_ADDR;
        if (status4 != SEND_IPV4_NOT_ATTEMPTED || status6 != SEND_IPV6_NOT_ATTEMPTED) {  // at least one invalid
            return status4 || status6;
        }
    } else if (ct4 == CAST_TYPE_NULL && ct6 == CAST_TYPE_NULL) {  // both are NULL
        return SEND_IPV4_ERR_INVALID_ADDR || SEND_IPV6_ERR_INVALID_ADDR;
    }

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
        status6 = HelperIPv6SendUDP(msg, encapsulated_binary_length, udp6, addr6, print_errors);
    }

    if (behaviour == SEND_IPV6_FIRST && status6 != SEND_IPV6_OK) {
        status4 = HelperIPv4SendUDP(msg, encapsulated_binary_length, udp4, addr4, print_errors);
    }

    return status4 || status6;
}

size_t ReceiveUDP(
    ReceivedMessage* recv_msg_struct,
    int udp
) {
    if (recv_msg_struct->buffer == NULL || recv_msg_struct->buffer_size < HEADER_LENGTH || udp < 0) {
        return 0;
    }
    memset(recv_msg_struct->buffer, 0, recv_msg_struct->buffer_size);
    char recv_binary[MAX_UDP_MESSAGE_SIZE];
    memset(recv_binary, 0, MAX_UDP_MESSAGE_SIZE);

    ssize_t recv_bytes = recvfrom(udp, recv_binary, MAX_UDP_MESSAGE_SIZE, 0, (struct sockaddr*) &recv_msg_struct->addr, sizeof(recv_msg_struct->addr));
    if (recv_bytes < 0) return 0;

    recv_msg_struct->msg_type = Deencapsulate(recv_binary, MAX_UDP_MESSAGE_SIZE, recv_msg_struct->buffer, recv_msg_struct->buffer_size);
    if (recv_msg_struct->msg_type == MSG_INVALID) return 0;
    return recv_bytes;
}
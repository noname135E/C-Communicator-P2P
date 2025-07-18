// Copyright 2025 Michał Jankowski
#ifndef SRC_SOCKET_PREPARATION_H_
#define SRC_SOCKET_PREPARATION_H_

typedef enum {
    SOCK_PREP_ERR_CREATE_FAIL = -1,
    SOCK_PREP_ERR_REUSEADDR_FAIL = -2,
    SOCK_PREP_ERR_BIND_TO_ADDR_FAIL = -3,
    SOCK_PREP_ERR_BINDTODEVICE_FAIL = -4,
    SOCK_PREP_ERR_IOCTL_FAIL = -5,
    SOCK_PREP_ERR_MULTICAST_IF_FAIL = -6,
    SOCK_PREP_ERR_MULTICAST_MEMBERSHIP_FAIL = -7,
    SOCK_PREP_ERR_IFINDEX_NOT_FOUND = -8,
    SOCK_PREP_ERR_JOIN_GROUP_FAIL = -9,
} SocketPreparationError;

/**
 * @brief Prepares multicast-capable IPv4 socket bound to specified interface.
 * @param ifname Name of the interface to bind to.
 * @return A non-negative file descriptor of the created socket or a negative error code (see SocketPreparationError enum).
 */
int GetInet4SocketUDP(const char *ifname);

/**
 * @brief Prepares multicast-capable IPv6 socket bound to specified interface.
 * @param ifname Name of the interface to bind to.
 * @return A non-negative file descriptor of the created socket or a negative error code (see SocketPreparationError enum).
 */
int GetInet6SocketUDP(const char *ifname);

#endif  // SRC_SOCKET_PREPARATION_H_

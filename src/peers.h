// Copyright 2025 Michał Jankowski
#ifndef SRC_PEERS_H_
#define SRC_PEERS_H_
#include <netinet/in.h>
#include <stdlib.h>
#include <time.h>

extern const size_t USER_IDENTIFIER_SIZE;
extern const size_t MAX_PEERS;

/**
 * @brief Creates a local user identifier.
 * @param username Username chosen by the user on startup.
 * @param user_identifier Buffer to store the created user identifier in.
 * @param user_identifier_size Size of the user identifier buffer.
 * @return 0 on success, -1 on failure.
 */
int CreateLocalUserIdentifier(
    const char* username,
    char* user_identifier,
    size_t user_identifier_size
);

typedef struct {
    struct sockaddr_in addr4;
    time_t last_seen;
} PeerIpv4Address;

typedef struct {
    struct sockaddr_in6 addr6;
    time_t last_seen;
} PeerIpv6Address;

typedef struct {
    PeerIpv4Address ipv4;
    PeerIpv6Address ipv6;
    char* user_identifier;
} Peer;

/**
 * @brief Prints the human-readable time.
 * @param time Time to print.
 */
void PrintHumanReadableTime(time_t time);

/**
 * @brief Prints the details of all peers.
 * @param peers Array of peers to print.
 * @param peers_size Size of the peers array.
 */
void PrintPeers(
    const Peer* peers,
    size_t peers_size
);

/**
 * @brief Finds a peer by its IPv4 address.
 * @param peers Array of peers to search in.
 * @param peers_size Size of the peers array.
 * @param addr4 Pointer to the IPv4 address to search for.
 * @return Index of the peer in the array if found, -1 if not found.
 */
ssize_t FindPeerByIpv4(
    Peer* peers,
    size_t peers_size,
    struct sockaddr_in* addr4
);

/**
 * @brief Finds a peer by its IPv6 address.
 * @param peers Array of peers to search in.
 * @param peers_size Size of the peers array.
 * @param addr6 Pointer to the IPv6 address to search for.
 * @return Index of the peer in the array if found, -1 if not found.
 */
ssize_t FindPeerByIpv6(
    Peer* peers,
    size_t peers_size,
    struct sockaddr_in6* addr6
);

/**
 * @brief Finds a peer by its user identifier.
 * @param peers Array of peers to search in.
 * @param peers_size Size of the peers array.
 * @param user_identifier User identifier to search for.
 * @return Index of the peer in the array if found, -1 if not found.
 */
ssize_t FindPeerByUserIdentifier(
    Peer* peers,
    size_t peers_size,
    const char* user_identifier
);

typedef enum {
    UPDATE_PEER_IPV4 = 0x1,
    UPDATE_PEER_IPV6 = 0x2,
} UpdatePeerFlags;

/**
 * @brief Updates or creates a peer entry based on the provided addresses and user identifier.
 * @param peers Array of peers to update.
 * @param peers_size Size of the peers array.
 * @param user_identifier User identifier for the peer.
 * @param addr4 Pointer to the IPv4 address to update or create. May be NULL if flags are set correctly.
 * @param addr6 Pointer to the IPv6 address to update or create. May be NULL if flags are set correctly.
 * @param flags Flags to determine which addresses to update or create.
 * @return Index of the updated or created peer in the array, or -1 on error.
 */
ssize_t UpdatePeer(
    Peer* peers,
    size_t peers_size,
    const char* user_identifier,
    struct sockaddr_in* addr4,
    struct sockaddr_in6* addr6,
    UpdatePeerFlags flags
);

#endif  // SRC_PEERS_H_

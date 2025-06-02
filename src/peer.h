// Copyright 2025 Michał Jankowski
#ifndef SRC_PEER_H_
#define SRC_PEER_H_

#include <netinet/in.h>
#include <sys/socket.h>
#include <time.h>

typedef struct {
    time_t seen;  // 0 = addr4 not assigned
    struct in_addr addr4;  // s_addr = 0 = addr4 not assigned
} SeenInet4;

typedef struct {
    time_t seen;  // 0 = addr6 not assigned
    struct in6_addr addr6;  // __in6_u.__u6_addr32[0] = 0 = addr6 not assigned
} SeenInet6;

// a list might be more flexible, but an array is more predictable
typedef struct {
    char user_identifier[320];
    SeenInet4 inet4;
    SeenInet6 inet6;
} Peer;

int FindByInet4(Peer peers[], const size_t peers_size, struct in_addr *addr4);
int FindByInet6(Peer peers[], const size_t peers_size, struct in6_addr *addr6);
int FindByUserIdentifier(Peer peers[], const size_t peers_size, const char *user_identifier);
int SetPeerInet4(Peer peers[], const size_t peers_size, struct in_addr *addr4, const char *user_identifier);
int SetPeerInet6(Peer peers[], const size_t peers_size, struct in6_addr *addr6, const char *user_identifier);
int CreatePeerAtPosition(
    Peer peers[],
    const size_t peers_size,
    int pos,
    struct in_addr *addr4,
    struct in6_addr *addr6,
    const char *user_identifier);
int RemovePeerAddressAtPosition(
    Peer peers[],
    const size_t peers_size,
    size_t pos,
    short remove_ipv4,
    short remove_ipv6);

void PrintPeers(Peer peers[], const size_t peers_size);
void PrintHumanReadableTime(time_t t);
void ClearAllPeers(Peer peers[], const size_t peers_size);

#endif  // SRC_PEER_H_

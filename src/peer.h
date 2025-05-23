// Copyright 2025 Michał Jankowski
#ifndef SRC_PEER_H_
#define SRC_PEER_H_

#include <netinet/in.h>
#include <sys/socket.h>
#include <time.h>

typedef struct {
    time_t seen;
    struct in_addr addr4;
} SeenInet4;

typedef struct {
    time_t seen;
    struct in6_addr addr6;
} SeenInet6;

// a list might be more flexible, but an array is more predictable
typedef struct {
    char user_identifier[320];
    SeenInet4 inet4;
    SeenInet6 inet6;
} Peer;

int FindByInet4(Peer peers[], const size_t peers_size, struct in_addr *addr4);
int FindByInet6(Peer peers[], const size_t peers_size, struct in6_addr *addr6);
int FindByUserIdentified(Peer peers[], const size_t peers_size, const char *user_identifier);
int SetPeerInet4(Peer peers[], const size_t peers_size, struct in_addr *addr4, const char *user_identifier);
int SetPeerInet6(Peer peers[], const size_t peers_size, struct in_addr *addr6, const char *user_identifier);

#endif  // SRC_PEER_H_

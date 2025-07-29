// Copyright 2025 Michał Jankowski
#ifndef SRC_NETWORK_CONTEXT_H_
#define SRC_NETWORK_CONTEXT_H_
#include "network.h"
#include "peers.h"

typedef struct {
    int udp4;
    int udp6;
    int ifindex;
    CurrentSendBehaviours* send_behaviours;
    char* user_identifier;
    Peer* peers;
    size_t peers_size;
} NetworkContext;
#endif  // SRC_NETWORK_CONTEXT_H_

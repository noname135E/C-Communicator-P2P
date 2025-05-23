// Copyright 2025 Michał Jankowski
#ifndef SRC_NET_FUNC_H_
#define SRC_NET_FUNC_H_

#include <stdint.h>

#include "peer.h"

long int Encapsulate(const uint8_t msg_type, char* msg, const size_t buf_size);
int Deencapsulate(char* msg);
void ListenUDP(int udp, Peer peers[], const size_t peers_size);
int SendScan(int udp4, int udp6, int ifindex, const char *user_identifier);

#endif  // SRC_NET_FUNC_H_

// Copyright 2025 Michał Jankowski
#ifndef SRC_NET_FUNC_H_
#define SRC_NET_FUNC_H_

#include <stdint.h>

#include "peer.h"

// enum MessageType {
//     SCAN,
//     SCAN_RESPONSE,
//     CLEARTEXT_MESSAGE,
//     DISCONNECT,
// };

long int Encapsulate(const enum MessageType msg_type, char* msg, const size_t buf_size);
int Deencapsulate(char* msg, ssize_t msg_length);
void ListenUDP(int udp, Peer peers[], const size_t peers_size, const char* user_identifier);
int SendScan(int udp4, int udp6, int ifindex, const char *user_identifier);
int SendScanResponse(int udp, const char *user_identifier, struct sockaddr_storage* src_addr, socklen_t src_addr_size);
void ProcessMessageScan(
    int udp,
    const char* user_idenitifer,
    Peer peers[],
    const size_t peers_size,
    char* msg,
    size_t msg_length,
    struct sockaddr_storage* src_addr,
    socklen_t src_addr_size
);
void ProcessMessageScanResponse(
    Peer peers[],
    const size_t peers_size,
    char* msg,
    size_t msg_length,
    struct sockaddr_storage* src_addr
);
void ProcessMessageCleartext(
    Peer peers[],
    const size_t peers_size,
    char* msg,
    struct sockaddr_storage* remote_addr
);
void ProcessMessageDisconnect(
    Peer peers[],
    const size_t peers_size,
    struct sockaddr_storage* remote_addr
);
int SendMsg(int udp4, int udp6, char* cmd, Peer peers[], size_t peers_size);
int SendDisconnect(int udp4, int udp6, Peer peers[], size_t peers_size, size_t id);
void SendDisconnectToAll(int udp4, int udp6, Peer peers[], size_t peers_size);
#endif  // SRC_NET_FUNC_H_

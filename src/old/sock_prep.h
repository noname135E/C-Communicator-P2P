// Copyright 2025 Michał Jankowski
#ifndef SRC_SOCK_PREP_H_
#define SRC_SOCK_PREP_H_

extern const char* MCAST_GROUP;
extern const char* MCAST6_GROUP;
extern const unsigned int PORT;

int GetInet4SocketUDP(const char *ifname);
int GetInet6SocketUDP(const char *ifname);

#endif  // SRC_SOCK_PREP_H_

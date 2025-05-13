// Copyright 2025 Michał Jankowski
#include <net/if.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "net_func.h"
#include "sock_prep.h"

int main(int argc, char *argv[]) {
    int udp4, udp6;

    if (argc != 2) {
        printf("Usage: c_comm [INTERFACE NAME]\n");
        exit(1);
    }
    int ifindex = if_nametoindex(argv[1]);
    if (ifindex == 0) {
        perror(argv[1]);
        exit(2);
    }

    if ((udp4 = GetInet4SocketUDP(argv[1])) < 0) {
        fprintf(stderr, "Failed to start IPv4/UDP communication, code %i\n", udp4);
    }
    if ((udp6 = GetInet6SocketUDP(argv[1])) < 0) {
        fprintf(stderr, "Failed to start IPv6/UDP communication, code %i\n", udp6);
    }
    sleep(10);
    return 0;
}

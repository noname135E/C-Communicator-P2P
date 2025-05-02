// Copyright 2025 Michał Jankowski
#include <net/if.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "sock_prep.h"

int main(int argc, char *argv[]) {
    int udp4, udp6;

    if (argc != 2) {
        printf("Usage: c_comm [INTERFACE NAME]\n");
        exit(1);
    }
    int iface_id = if_nametoindex(argv[1]);
    if (iface_id == 0) {
        perror(argv[1]);
        exit(2);
    }
    if ((udp4 = GetInet4SocketUDP(argv[1])) < 0) {
        fprintf(stderr, "Failed to start IPv4/UDP communication, code %i\n", udp4);
    }
    sleep(10);
    return 0;
}

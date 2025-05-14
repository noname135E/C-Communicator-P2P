// Copyright 2025 Michał Jankowski
#include <net/if.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "net_func.h"
#include "sock_prep.h"

int main(int argc, char *argv[]) {
    int udp4, udp6;
    char hostname[256];
    char user_identifier[320];

    if (argc != 3) {
        printf("Usage: c_comm [INTERFACE NAME] [USER NAME]\n");
        exit(1);
    }
    int ifindex = if_nametoindex(argv[1]);
    if (ifindex == 0) {
        perror(argv[1]);
        exit(2);
    }

    if (strlen(argv[2]) > 62) {
        fprintf(stderr, "User name must be no longer than 62 bytes.\n");
        exit(3);
    }
    if (gethostname(hostname, sizeof(hostname)) < 0) {
        perror("[FAIL] Could not get hostname");
        exit(4);
    }

    snprintf(user_identifier, sizeof(user_identifier), "%s@%s", argv[2], hostname);
    printf("This user/instance will be identified as: \"%s\"\n", user_identifier);

    if ((udp4 = GetInet4SocketUDP(argv[1])) < 0) {
        fprintf(stderr, "Failed to start IPv4/UDP communication, code %i\n", udp4);
    }
    if ((udp6 = GetInet6SocketUDP(argv[1])) < 0) {
        fprintf(stderr, "Failed to start IPv6/UDP communication, code %i\n", udp6);
    }
    sleep(10);
    return 0;
}

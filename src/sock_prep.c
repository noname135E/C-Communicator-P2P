// Copyright 2025 Michał Jankowski
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "sock_prep.h"

const char* MCAST_GROUP = "230.220.210.200";
const unsigned int PORT = 8192;

int GetInet4SocketUDP(const char *ifname) {
    int sockfd;
    struct sockaddr_in bind_addr;
    struct ifreq ifr;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        return -1;
    }

    memset(&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = INADDR_ANY;
    bind_addr.sin_port = htons(PORT);

    if ((bind(sockfd, (const struct sockaddr *) &bind_addr, sizeof(bind_addr))) < 0) {
        close(sockfd);
        return -2;
    }

    if ((setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, ifname, strlen(ifname) + 1)) < 0) {
        close(sockfd);
        return -3;
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
    if (ioctl(sockfd, SIOCGIFADDR, &ifr) < 0) {
        close(sockfd);
        return -4;
    }

    struct in_addr if_addr = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;
    if (setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_IF, &if_addr, sizeof(if_addr)) < 0) {
        close(sockfd);
        return -5;
    }

    return sockfd;
}

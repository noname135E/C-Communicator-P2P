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

const char* MCAST_GROUP = "224.0.0.192";
const char* MCAST6_GROUP = "ff02::C0";
const unsigned int PORT = 8192;

int GetInet4SocketUDP(const char *ifname) {
    int sockfd;
    struct sockaddr_in bind_addr;
    struct ifreq ifr;
    struct ip_mreq mreq;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        return -1;
    }

    memset(&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = INADDR_ANY;
    bind_addr.sin_port = htons(PORT);

    const unsigned int optval0 = 0;
    const unsigned int optval1 = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval1, sizeof(optval1)) < 0) {
        close(sockfd);
        return -5;
    }

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

    mreq.imr_multiaddr.s_addr = inet_addr(MCAST_GROUP);
    mreq.imr_interface.s_addr = if_addr.s_addr;
    if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        close(sockfd);
        return -6;
    }


    setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_LOOP, &optval0, sizeof(optval0));

    return sockfd;
}

int GetInet6SocketUDP(const char *ifname) {
    int sockfd;
    struct sockaddr_in6 bind_addr;
    struct ipv6_mreq mreq;

    if ((sockfd = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
        return -1;
    }

    memset(&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sin6_family = AF_INET6;
    bind_addr.sin6_addr = in6addr_any;
    bind_addr.sin6_port = htons(PORT);

    const unsigned int optval0 = 0;
    const unsigned int optval1 = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval1, sizeof(optval1)) < 0) {
        close(sockfd);
        return -6;
    }

    if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &optval1, sizeof(optval1)) < 0) {
        // Done as an if to make it easier to add logging in the future
        // IPv4 support in an IPv6 socket hopefully should not break anything
        // => no need to disable IPv6 communication if this fails
    }

    if ((bind(sockfd, (const struct sockaddr *) &bind_addr, sizeof(bind_addr))) < 0) {
        close(sockfd);
        return -2;
    }

    if ((setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, ifname, strlen(ifname) + 1)) < 0) {
        close(sockfd);
        return -3;
    }

    unsigned int ifindex = if_nametoindex(ifname);
    if (ifindex == 0) {
        close(sockfd);
        return -4;
    }

    if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifindex, sizeof(ifindex)) < 0) {
        close(sockfd);
        return -5;
    }

    memset(&mreq, 0, sizeof(mreq));
    if (inet_pton(AF_INET6, MCAST6_GROUP, &mreq.ipv6mr_multiaddr) != 1) {
        close(sockfd);
        return -7;
    }

    mreq.ipv6mr_interface = ifindex;
    if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_JOIN_GROUP, &mreq, sizeof(mreq)) < 0) {
        close(sockfd);
        return -8;
    }

    setsockopt(sockfd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &optval0, sizeof(optval0));

    return sockfd;
}

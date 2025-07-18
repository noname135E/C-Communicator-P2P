// Copyright 2025 Michał Jankowski
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "socket_preparation.h"

const char* MCAST4_GROUP = "224.0.0.192";
const char* MCAST6_GROUP = "ff02::C0";
const unsigned int PORT = 8192;

int GetInet4SocketUDP(const char *ifname) {
    int sockfd;
    struct sockaddr_in bind_addr;
    struct ifreq ifr;
    struct ip_mreq mreq;
    const unsigned int optval0 = 0;
    const unsigned int optval1 = 1;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        return SOCK_PREP_ERR_CREATE_FAIL;
    }

    memset(&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = INADDR_ANY;
    bind_addr.sin_port = htons(PORT);

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval1, sizeof(optval1)) < 0) {
        close(sockfd);
        return SOCK_PREP_ERR_REUSEADDR_FAIL;
    }

    if ((bind(sockfd, (const struct sockaddr *) &bind_addr, sizeof(bind_addr))) < 0) {
        close(sockfd);
        return SOCK_PREP_ERR_BIND_TO_ADDR_FAIL;
    }

    if ((setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, ifname, strlen(ifname) + 1)) < 0) {
        close(sockfd);
        return SOCK_PREP_ERR_BINDTODEVICE_FAIL;
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
    if (ioctl(sockfd, SIOCGIFADDR, &ifr) < 0) {
        close(sockfd);
        return SOCK_PREP_ERR_IOCTL_FAIL;
    }

    struct in_addr if_addr = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;
    if (setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_IF, &if_addr, sizeof(if_addr)) < 0) {
        close(sockfd);
        return SOCK_PREP_ERR_MULTICAST_IF_FAIL;
    }

    mreq.imr_multiaddr.s_addr = inet_addr(MCAST4_GROUP);
    mreq.imr_interface.s_addr = if_addr.s_addr;
    if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        close(sockfd);
        return SOCK_PREP_ERR_MULTICAST_MEMBERSHIP_FAIL;
    }

    // loop not being disabled should be fine
    setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_LOOP, &optval0, sizeof(optval0));
    return sockfd;
}

int GetInet6SocketUDP(const char *ifname) {
    int sockfd;
    struct sockaddr_in6 bind_addr;
    struct ipv6_mreq mreq;
    const unsigned int optval0 = 0;
    const unsigned int optval1 = 1;

    if ((sockfd = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
        return SOCK_PREP_ERR_CREATE_FAIL;
    }

    memset(&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sin6_family = AF_INET6;
    bind_addr.sin6_addr = in6addr_any;
    bind_addr.sin6_port = htons(PORT);

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval1, sizeof(optval1)) < 0) {
        close(sockfd);
        return SOCK_PREP_ERR_REUSEADDR_FAIL;
    }

    // IPv4 support in an IPv6 socket hopefully should not break anything
    // => no need to disable IPv6 communication if this fails
    setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &optval1, sizeof(optval1));

    if ((bind(sockfd, (const struct sockaddr *) &bind_addr, sizeof(bind_addr))) < 0) {
        close(sockfd);
        return SOCK_PREP_ERR_BIND_TO_ADDR_FAIL;
    }

    if ((setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, ifname, strlen(ifname) + 1)) < 0) {
        close(sockfd);
        return SOCK_PREP_ERR_BINDTODEVICE_FAIL;
    }

    unsigned int ifindex = if_nametoindex(ifname);
    if (ifindex == 0) {
        close(sockfd);
        return SOCK_PREP_ERR_IFINDEX_NOT_FOUND;
    }

    if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifindex, sizeof(ifindex)) < 0) {
        close(sockfd);
        return SOCK_PREP_ERR_MULTICAST_IF_FAIL;
    }

    memset(&mreq, 0, sizeof(mreq));
    if (inet_pton(AF_INET6, MCAST6_GROUP, &mreq.ipv6mr_multiaddr) != 1) {
        close(sockfd);
        return SOCK_PREP_ERR_MULTICAST_MEMBERSHIP_FAIL;
    }

    mreq.ipv6mr_interface = ifindex;
    if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_JOIN_GROUP, &mreq, sizeof(mreq)) < 0) {
        close(sockfd);
        return SOCK_PREP_ERR_JOIN_GROUP_FAIL;
    }

    // loop not being disabled should be fine
    setsockopt(sockfd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &optval0, sizeof(optval0));
    return sockfd;
}

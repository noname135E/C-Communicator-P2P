// Copyright 2025 Michał Jankowski
#include <net/if.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <poll.h>
#include "unistd.h"

#include "common.h"
#include "command_processor.h"
#include "peers.h"
#include "network.h"
#include "socket_preparation.h"

const nfds_t MAX_POLL_FDS = 5;
const unsigned int POLL_TIMEOUT_MS = 100;
const char* LOCKFILE_DIR = "/var/lock";

const size_t MSG_BUFFER_SIZE = 2048;

static inline ssize_t AddPollFd(
    struct pollfd *fds,
    nfds_t *nfds,
    int fd,
    short events
) {
    if (*nfds >= MAX_POLL_FDS) {
        fprintf(stderr, "[ ERR] Failed to register file descriptor in pollfd.\n");
        return -1;
    }
    fds[*nfds].fd = fd;
    fds[*nfds].events = events;
    nfds_t ret = *nfds;
    (*nfds)++;
    return (ssize_t) ret;
}

static inline int CloseAllFds(
    struct pollfd *fds,
    nfds_t nfds,
    char* lockfile
) {
    for (nfds_t i = 0; i < nfds; i++) {
        if (fds[i].fd >= 0) {
            close(fds[i].fd);
        }
    }
    unlink(lockfile);
    printf("Goodbye!\n");
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    char stdin_buffer[MSG_BUFFER_SIZE];
    char user_identifier[USER_IDENTIFIER_SIZE];
    char* ifname;
    int udp4, udp6;
    ssize_t udp4_index = -1, udp6_index = -1;
    CmdReturnSignal cmd_signal;
    Peer peers[MAX_PEERS];
    memset(peers, 0, sizeof(peers));

    CurrentSendBehaviours send_behaviours = {
        .both = SEND_BOTH,
        .ipv4_first = SEND_IPV4_FIRST,
        .ipv6_first = SEND_IPV6_FIRST,
    };

    NetworkContext net_context = {
        .udp4 = -1,
        .udp6 = -1,
        .ifindex = 0,
        .send_behaviours = &send_behaviours,
        .user_identifier = user_identifier,
        .peers = peers,
        .peers_size = MAX_PEERS
    };

    struct pollfd fds[MAX_POLL_FDS];
    memset(fds, 0, sizeof(fds));
    nfds_t nfds = 0;

    if (argc != 3) {
        fprintf(stderr, "Usage: c_comm [INTERFACE NAME] [USER NAME]\n");
        exit(EXIT_FAILURE);
    }

    ifname = argv[1];
    net_context.ifindex = if_nametoindex(ifname);
    if (net_context.ifindex == 0) {
        fprintf(stderr, "[FAIL] Could not find interface: %s: ", ifname);
        perror("");
        exit(EXIT_FAILURE);
    }
    printf("[ OK ] Found interface \"%s\".\n", ifname);

    int lock_fd = -1;
    char lockfile[256];
    snprintf(lockfile, sizeof(lockfile), "%s/c_comm_%s.lock", LOCKFILE_DIR, argv[1]);
    lock_fd = open(lockfile, O_CREAT | O_RDWR, 0644);
    if (lock_fd < 0) {
        perror("[FAIL] Could not obtain lock file. Unable to confirm if another instance is running. Exiting.\n");
        exit(EXIT_FAILURE);
    } else if (flock(lock_fd, LOCK_EX | LOCK_NB) < 0) {
        fprintf(stderr, "[FAIL] Another instance is already running on this interface. Cannot proceed. Exiting.\n");
        close(lock_fd);
        exit(EXIT_FAILURE);
    }
    printf("[ OK ] Confirmed lack of other instances.\n");

    if (CreateLocalUserIdentifier(argv[2], user_identifier, sizeof(user_identifier)) < 0) {
        fprintf(stderr, "[FAIL] Could not create user identifier. Exiting.\n");
        close(lock_fd);
        exit(EXIT_FAILURE);
    }
    printf("[ OK ] Successfully created user identifier.\n");


    AddPollFd(fds, &nfds, STDIN_FILENO, POLLIN);
    if ((udp4 = GetUDP4Socket(ifname)) < 0) {
        fprintf(stderr, "[WARN] Could not create IPv4/UDP socket.\n");
        udp4 = -999;  // to avoid issues in poll loop
        // (can't think of how that problem would occur, just for peace of mind)
        send_behaviours.both = SEND_IPV6_ONLY;
        send_behaviours.ipv4_first = SEND_IPV6_ONLY;
    } else {
        udp4_index = AddPollFd(fds, &nfds, udp4, POLLIN);
        if (udp4_index < 0) {
            fprintf(stderr, "[ ERR] Could not register IPv4/UDP socket in pollfd. Exiting.\n");
            close(udp4);
            close(lock_fd);
            exit(EXIT_FAILURE);
        }
        net_context.udp4 = udp4;
        printf("[ OK ] Successfully created IPv4/UDP socket.\n");
    }
    if ((udp6 = GetUDP6Socket(ifname)) < 0) {
        fprintf(stderr, "[WARN] Could not create IPv6/UDP socket.\n");
        udp6 = -999;  // to avoid issues in poll loop
        // Check for no IPv4 and no IPv6 is present later
        send_behaviours.both = SEND_IPV4_ONLY;
        send_behaviours.ipv6_first = SEND_IPV4_ONLY;
    } else {
        udp6_index = AddPollFd(fds, &nfds, udp6, POLLIN);
        if (udp6_index < 0) {
            fprintf(stderr, "[ ERR] Could not register IPv6/UDP socket in pollfd. Exiting.\n");
            close(udp6);
            if (udp4 >= 0) {
                close(udp4);
            }
            close(lock_fd);
            exit(EXIT_FAILURE);
        }
        net_context.udp6 = udp6;
        printf("[ OK ] Successfully created IPv6/UDP socket.\n");
    }
    if (udp4 < 0 && udp6 < 0) {
        fprintf(stderr, "[FAIL] Could not create any UDP socket. Exiting.\n");
        close(lock_fd);
        exit(EXIT_FAILURE);
    }

    printf("[ OK ] Finished initialization.\n");
    printf("=========================================================\n\n");
    printf("Hello!\nThis user/instance will be identified as: \"%s\"\n", user_identifier);
    printf("Type \"/help\" for a list of commands or \"/exit\" to exit the application.\n\n");

    while (true) {
        int ret = poll(fds, nfds, POLL_TIMEOUT_MS);

        if (ret < 0) {
            perror("[ ERR] Poll");
            return EXIT_FAILURE;
        } else if (ret > 0) {
            for (nfds_t i = 0; i < nfds; i++) {
                if (fds[i].revents == STDIN_FILENO) {  // handle user input
                    fgets(stdin_buffer, sizeof(stdin_buffer), stdin);
                    cmd_signal = HandleCommand(stdin_buffer, &net_context);
                    switch (cmd_signal) {
                        case CMD_RETURN_EXIT:
                            return CloseAllFds(fds, nfds, lockfile);
                            break;
                        default:
                            break;
                    }
                }
            }
        }

        // remove fds closed in poll loop
        nfds_t j = 0;
        bool found_udp4 = false, found_udp6 = false;
        for (nfds_t i = 0; i < nfds; i++) {
            if (fds[i].fd != -1) {
                if (i != j) {
                    fds[j] = fds[i];
                }
                if (fds[j].fd == net_context.udp4) {
                    found_udp4 = true;
                    udp4_index = j;
                } else if (fds[j].fd == net_context.udp6) {
                    found_udp6 = true;
                    udp6_index = j;
                }
                j++;
            }
        }
        nfds = j;
        if (udp4_index != -1 && !found_udp4) {
            net_context.udp4 = -1;
            udp4_index = -1;
            // TODO(.): Add send_behaviour updates.
        }
        if (udp6_index != -1 && !found_udp6) {
            net_context.udp6 = -1;
            udp6_index = -1;
        }

        if (udp4_index == -1 && udp6_index == -1) {
            fprintf(stderr, "[ ERR] No UDP sockets available. Exiting.\n");
            CloseAllFds(fds, nfds, lockfile);
            return EXIT_FAILURE;
        }
    }

    return CloseAllFds(fds, nfds, lockfile);
}

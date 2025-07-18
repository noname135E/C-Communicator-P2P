// Copyright 2025 Michał Jankowski
#include <fcntl.h>
#include <net/if.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <poll.h>
#include <unistd.h>

#include "common.h"
#include "net_func.h"
#include "peer.h"
#include "sock_prep.h"

const unsigned int MAX_POLL_FDS = 5;
const unsigned int POLL_TIMEOUT_MS = 100;
const char* LOCKFILE_DIR = "/var/lock";

const size_t MSG_BUFFER_SIZE = 2048;
const size_t NET_BUFFER_SIZE = MSG_BUFFER_SIZE + 2;

static inline void AddPollFd(
    struct pollfd *fds,
    unsigned int *nfds,
    int fd,
    short events
) {
    if (*nfds >= MAX_POLL_FDS) {
        fprintf(stderr, "[ ERR] Failed to register file descriptor in pollfd.\n");
        return;
    }
    fds[*nfds].fd = fd;
    fds[*nfds].events = events;
    (*nfds)++;
}

enum Command DetermineCommand(char *cmd_string) {
    enum Command output = CMD_UNKNOWN;
    if (strcmp(cmd_string, "/exit") == 0) {
        output = CMD_EXIT;
    } else if (strcmp(cmd_string, "/help") == 0) {
        output = CMD_HELP;
    } else if (strcmp(cmd_string, "/clear") == 0) {
        output = CMD_CLEAR_ALL;
    } else if (strcmp(cmd_string, "/list") == 0 || strcmp(cmd_string, "/peers") == 0) {
        output = CMD_PRINT_PEERS;
    } else if (strcmp(cmd_string, "/scan") == 0) {
        output = CMD_SCAN;
    } else if (strncmp(cmd_string, "/send ", 6) == 0) {
        output = CMD_SEND;
    } else if (strncmp(cmd_string, "/send ", 5) == 0) {
        printf("Usage: /send [PEER ID] [MESSAGE]\n");
        output = CMD_SILENT;
    } else if (strcmp(cmd_string, "/disconnect") == 0) {
        output = CMD_DISCONNECT_ALL;
    } else if (strcmp(cmd_string, "/whoami") == 0) {
        output = CMD_WHOAMI;
    }
    return output;
}

void PrintHelp() {
    printf("/help       - displays help\n");
    printf("/exit       - exits application\n");
    // printf("/clear  - clears all peers\n");
    printf("/disconnect - disconnects all peers\n");
    printf("/list       - prints peers\n");
    printf("/scan       - scans network in search of peers\n");
    printf("/send       - send message to peer\n");
    printf("      Usage: /send [PEER ID] [MESSAGE]\n");
    printf("/whoami     - prints own user identifier");
    printf("\n");
}

int main(int argc, char *argv[]) {
    int udp4, udp6;
    char hostname[256];
    char user_identifier[320];
    char stdin_buffer[2048];
    unsigned short run = 1;

    struct pollfd fds[MAX_POLL_FDS];
    unsigned int nfds = 0;  // also current length, but next id feels better

    const size_t PEERS_SIZE = 32;
    Peer peers[PEERS_SIZE];
    memset(peers, 0, sizeof(peers));

    if (argc != 3) {
        printf("Usage: c_comm [INTERFACE NAME] [USER NAME]\n");
        exit(EXIT_FAILURE);
    }
    int ifindex = if_nametoindex(argv[1]);
    if (ifindex == 0) {
        perror(argv[1]);
        exit(EXIT_FAILURE);
    }

    if (strlen(argv[2]) > 62) {
        fprintf(stderr, "User name must be no longer than 62 bytes.\n");
        exit(EXIT_FAILURE);
    }
    if (gethostname(hostname, sizeof(hostname)) < 0) {
        perror("[FAIL] Could not get hostname");
        exit(EXIT_FAILURE);
    }

    int lock_fd = -1;
    char lockfile[256];
    snprintf(lockfile, sizeof(lockfile), "%s/c_comm_%s_%d.lock", LOCKFILE_DIR, argv[1], PORT);
    lock_fd = open(lockfile, O_CREAT | O_RDWR, 0644);
    if (lock_fd < 0) {
        perror("[FAIL] Could not obtain lockfile\n");
        exit(EXIT_FAILURE);
    } else if (flock(lock_fd, LOCK_EX | LOCK_NB) < 0) {
        fprintf(stderr, "[FAIL] Another instance is already running on this interface. Cannot proceed.\n");
        close(lock_fd);
        exit(EXIT_FAILURE);
    }

    snprintf(user_identifier, sizeof(user_identifier), "%s@%s", argv[2], hostname);
    printf("This user/instance will be identified as: \"%s\"\n", user_identifier);
    printf("For list of commands type \"/help\"\n\n");

    AddPollFd(fds, &nfds, STDIN_FILENO, POLLIN);
    if ((udp4 = GetInet4SocketUDP(argv[1])) < 0) {
        fprintf(stderr, "[WARN] Failed to start IPv4/UDP communication, code %i\n", udp4);
        udp4 = -999;  // to avoid issues in poll loop
        // (can't think of how that problem would occur, just for peace of mind)
    } else {
        AddPollFd(fds, &nfds, udp4, POLLIN);
    }
    if ((udp6 = GetInet6SocketUDP(argv[1])) < 0) {
        fprintf(stderr, "[WARN] Failed to start IPv6/UDP communication, code %i\n", udp6);
        udp6 = -999;  // to avoid issues in poll loop
    } else {
        AddPollFd(fds, &nfds, udp6, POLLIN);
    }

    if (udp4 < 0 && udp6 < 0) {
        fprintf(stderr, "[FAIL] Could not start UDP communication. Exiting.\n");
        exit(EXIT_FAILURE);
    }

    while (run) {
        int ret = poll(fds, nfds, POLL_TIMEOUT_MS);

        if (ret < 0) {
            perror("[FAIL] Poll");
            return EXIT_FAILURE;
        }
        if (ret > 0) {
            for (unsigned int i = 0; i < nfds; i++) {
                if (fds[i].revents & POLLIN) {
                    if (fds[i].fd == STDIN_FILENO) {  // handle user input
                        fgets(stdin_buffer, sizeof(stdin_buffer), stdin);
                        stdin_buffer[strcspn(stdin_buffer, "\n")] = '\0';
                        if (stdin_buffer[0] == '/') {
                            switch (DetermineCommand(stdin_buffer)) {
                                case CMD_UNKNOWN:
                                    printf("Unknown command.\n");
                                    break;
                                case CMD_EXIT:
                                    printf("Exiting...\n");
                                    run = 0;
                                    SendDisconnectToAll(udp4, udp6, peers, PEERS_SIZE);
                                    printf("Sent disconnects to all peers.\n");
                                    close(udp4);
                                    close(udp6);
                                    printf("Goodbye!\n");
                                    return EXIT_SUCCESS;
                                case CMD_HELP:
                                    PrintHelp();
                                    break;
                                case CMD_CLEAR_ALL:
                                    ClearAllPeers(peers, PEERS_SIZE);
                                    printf("Cleared all peers.\n");
                                    break;
                                case CMD_PRINT_PEERS:
                                    PrintPeers(peers, PEERS_SIZE);
                                    break;
                                case CMD_SCAN:
                                    printf("Sent scans.\n");
                                    SendScan(udp4, udp6, ifindex, user_identifier);
                                    break;
                                case CMD_SEND:
                                    SendMsg(udp4, udp6, stdin_buffer, peers, PEERS_SIZE);
                                    break;
                                case CMD_DISCONNECT_ALL:
                                    printf("Sending disconnects to all peers.\n");
                                    SendDisconnectToAll(udp4, udp6, peers, PEERS_SIZE);
                                    ClearAllPeers(peers, PEERS_SIZE);
                                    printf("Cleared all peers.\n");
                                    break;
                                case CMD_WHOAMI:
                                    printf("You are: \"%s\"\n", user_identifier);
                                    break;
                                default:
                                    break;
                            }
                        }
                        if (strcmp(stdin_buffer, "/exit") == 0) {
                            break;
                        }
                    } else if (fds[i].fd == udp4 || fds[i].fd == udp6) {  // handle IPv4/UDP and IPv6/UDP sockets
                        ListenUDP(fds[i].fd, peers, PEERS_SIZE, user_identifier);
                    }
                }
            }
        }

        unsigned int j = 0;
        for (unsigned int i = 0; i < nfds; i++) {
            if (fds[i].fd != -1) {
                if (i != j) {
                    fds[j] = fds[i];
                }
                j++;
            }
        }
        nfds = j;
    }

    close(udp4);
    close(udp6);
    return EXIT_SUCCESS;
}

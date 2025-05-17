// Copyright 2025 Michał Jankowski
#include <net/if.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <poll.h>
#include <unistd.h>

#include "net_func.h"
#include "sock_prep.h"

const unsigned int MAX_POLL_FDS = 5;
const unsigned int POLL_TIMEOUT_MS = 100;

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

enum Command {
    UNKNOWN,
    HELP,
    EXIT
};

enum Command DetermineCommand(char *cmd_string) {
    enum Command output = UNKNOWN;
    if (strcmp(cmd_string, "/exit") == 0) {
        output = EXIT;
    } else if (strcmp(cmd_string, "/help") == 0) {
        output = HELP;
    }
    return output;
}

void PrintHelp() {
    printf("/help - displays help\n");
    printf("/exit - exits application\n");
    printf("\n");
}

int main(int argc, char *argv[]) {
    int udp4, udp6;
    char hostname[256];
    char user_identifier[320];
    char stdin_buffer[1024];
    unsigned short run = 1;

    struct pollfd fds[MAX_POLL_FDS];
    unsigned int nfds = 0;  // also current length, but next id feels better

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

    snprintf(user_identifier, sizeof(user_identifier), "%s@%s", argv[2], hostname);
    printf("This user/instance will be identified as: \"%s\"\n", user_identifier);
    printf("For list of commands type \"/help\"\n\n");

    AddPollFd(fds, &nfds, STDIN_FILENO, POLLIN);
    if ((udp4 = GetInet4SocketUDP(argv[1])) < 0) {
        fprintf(stderr, "Failed to start IPv4/UDP communication, code %i\n", udp4);
        udp4 = -999;  // to avoid issues in poll loop
        // (can't think of how that problem would occur, just for peace of mind)
    } else {
        AddPollFd(fds, &nfds, udp4, POLLIN);
    }
    if ((udp6 = GetInet6SocketUDP(argv[1])) < 0) {
        fprintf(stderr, "Failed to start IPv6/UDP communication, code %i\n", udp6);
        udp6 = -999;  // to avoid issues in poll loop
    } else {
        AddPollFd(fds, &nfds, udp6, POLLIN);
    }

    while (run) {
        int ret = poll(fds, nfds, POLL_TIMEOUT_MS);

        if (ret < 0) {
            perror("[FAIL] Poll");
            return EXIT_FAILURE;
        } else if (ret != 0) {
            for (unsigned int i = 0; i < nfds; i++) {
                if (fds[i].revents & POLLIN) {
                    if (fds[i].fd == STDIN_FILENO) {  // handle user input
                        fgets(stdin_buffer, sizeof(stdin_buffer), stdin);
                        stdin_buffer[strcspn(stdin_buffer, "\n")] = '\0';
                        if (stdin_buffer[0] == '/') {
                            switch (DetermineCommand(stdin_buffer)) {
                                case UNKNOWN:
                                    printf("Unknown command.\n");
                                    break;
                                case EXIT:
                                    printf("Exiting...\n");
                                    run = 0;
                                    close(udp4);
                                    close(udp6);
                                    return EXIT_SUCCESS;
                                case HELP:
                                    PrintHelp();
                                    break;
                            }
                        }
                        if (strcmp(stdin_buffer, "/exit") == 0) {
                            break;
                        }
                    } else if (fds[i].fd == udp4) {  // handle IPv4/UDP socket
                        continue;  // TODO(.): Implement
                    } else if (fds[i].fd == udp6) {  // handle IPv6/UDP socket
                        continue;  // TODO(.): Implement
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

    return EXIT_SUCCESS;
}

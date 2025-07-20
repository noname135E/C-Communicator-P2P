// Copyright 2025 Michał Jankowski
#include <net/if.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/file.h>
#include "unistd.h"

#include "common.h"
#include "command_processor.h"
#include "peers.h"
#include "network.h"
#include "socket_preparation.h"

const unsigned int MAX_POLL_FDS = 5;
const unsigned int POLL_TIMEOUT_MS = 100;
const char* LOCKFILE_DIR = "/var/lock";

const size_t MSG_BUFFER_SIZE = 2048;

int main(int argc, char *argv[]) {
    char stdin_buffer[MSG_BUFFER_SIZE];
    char user_identifier[USER_IDENTIFIER_SIZE];
    char* ifname;
    int ifindex, udp4, udp6;
    CmdReturnSignal cmd_signal;

    if (argc != 3) {
        fprintf(stderr, "Usage: c_comm [INTERFACE NAME] [USER NAME]\n");
        exit(EXIT_FAILURE);
    }

    ifname = argv[1];
    ifindex = if_nametoindex(ifname);
    if (ifindex == 0) {
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
        exit(EXIT_FAILURE);
    }
    printf("[ OK ] Successfully created user identifier.\n");


    // TODO(.): Consider getting a struct to adjust behaviours of sending here.
    if ((udp4 = GetUDP4Socket(ifname)) < 0) {
        fprintf(stderr, "[WARN] Could not create IPv4/UDP socket.\n");
        udp4 = -999;  // to avoid issues in poll loop
    } else {
        // TODO(.): Add poll fd adding
        printf("[ OK ] Successfully created IPv4/UDP socket.\n");
    }
    if ((udp6 = GetUDP6Socket(ifname)) < 0) {
        fprintf(stderr, "[WARN] Could not create IPv6/UDP socket.\n");
        udp6 = -999;  // to avoid issues in poll loop
    } else {
        // TODO(.): Add poll fd adding
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
    while (1) {
        fgets(stdin_buffer, sizeof(stdin_buffer), stdin);
        cmd_signal = HandleCommand(stdin_buffer);
        if (cmd_signal == CMD_RETURN_EXIT) {
            printf("Goodbye!\n");
            unlink(lockfile);
            return 0;
        }
    }
}

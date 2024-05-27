#ifndef FTP_COMMON_H
#define FTP_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

void convert_port_to_p1_p2(int port, int *p1, int *p2) {
    *p1 = port / 256;
    *p2 = port % 256;
}

int convert_p1_p2_to_port(int p1, int p2) {
    return (p1 * 256) + p2;
}

#endif

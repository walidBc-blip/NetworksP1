#ifndef FTP_CLIENT_H
#define FTP_CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/select.h>


#define BUFFER_CAPACITY 2048
#define CTRL_PORT 2121
#define DATA_CHANNEL_PORT 2020

extern bool serverActive;
extern int authStatus;

int initializeSocket(bool listenMode, int portNumber);
void handleClientCommand(char *commandBuffer, int controlSocketFd);
char *generatePortString(const char *ip, int port);
char *transmitToServer(int socketFd, const char *buffer);
bool configurePortCommand(int socketFd, int channel);

void executeLocalPWD();
void executeLocalCWD(char *buffer);
void executeLocalLIST();

#endif // FTP_CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include <netinet/in.h> 
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/select.h>

#define BUFFER_CAPACITY 2048
#define CTRL_PORT 2121
#define DATA_CHANNEL_PORT 2020

bool serverActive = true;
int authStatus = -1;

int initializeSocket(bool listenMode, int portNumber);
void handleClientCommand(char *commandBuffer, int controlSocketFd);
char *generatePortString(const char *ip, int port);
int generatePort();
char *transmitToServer(int socketFd, const char *buffer);
bool configurePortCommand(int socketFd, int channel);
void executeLocalPWD();
void executeLocalCWD(char *buffer);
void executeLocalLIST();

int main() {
    int controlSocketFd = initializeSocket(false, CTRL_PORT);
    char commandBuffer[BUFFER_CAPACITY];

    while (serverActive) {
        printf("ftp> ");
        if (fgets(commandBuffer, BUFFER_CAPACITY, stdin) != NULL) {
            commandBuffer[strcspn(commandBuffer, "\n")] = '\0';
            handleClientCommand(commandBuffer, controlSocketFd);
            memset(commandBuffer, 0, BUFFER_CAPACITY);
        } else {
            perror("Error reading input");
            break;
        }
    }

    close(controlSocketFd);
    return 0;
}

int initializeSocket(bool listenMode, int portNumber) {
    int sockFd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockFd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int optionValue = 1;
    setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &optionValue, sizeof(optionValue));

    struct sockaddr_in socketAddress;
    memset(&socketAddress, 0, sizeof(socketAddress));
    socketAddress.sin_family = AF_INET;
    socketAddress.sin_port = htons(portNumber);
    socketAddress.sin_addr.s_addr = INADDR_ANY;

    if (listenMode) {
        if (bind(sockFd, (struct sockaddr *)&socketAddress, sizeof(socketAddress)) < 0) {
            perror("Socket binding failed");
            close(sockFd);
            exit(EXIT_FAILURE);
        }

        if (listen(sockFd, 5) < 0) {
            perror("Listening failed");
            close(sockFd);
            exit(EXIT_FAILURE);
        }
    } else {
        if (connect(sockFd, (struct sockaddr *)&socketAddress, sizeof(socketAddress)) == -1) {
            perror("Connection failed");
            close(sockFd);
            exit(EXIT_FAILURE);
        }
    }

    return sockFd;
}

bool configurePortCommand(int socketFd, int channel) {
    struct sockaddr_in address;
    socklen_t addressLength = sizeof(address);

    if (getsockname(channel, (struct sockaddr *)&address, &addressLength) == -1) {
        perror("Failed to get socket name");
        return false;
    }

    char *ipAddress = inet_ntoa(address.sin_addr);
    int portNumber = ntohs(address.sin_port);
    char *portMessage = generatePortString(ipAddress, portNumber);
    char commandBuffer[BUFFER_CAPACITY];
    snprintf(commandBuffer, BUFFER_CAPACITY, "PORT %s", portMessage);

    char *responseCode = transmitToServer(socketFd, commandBuffer);
    bool isCommandSuccessful = strstr(responseCode, "200") != NULL;

    free(portMessage);
    free(responseCode);

    return isCommandSuccessful;
}

void handleClientCommand(char *commandBuffer, int controlSocketFd) {
    char command[6] = {0};
    strncpy(command, commandBuffer, 5);

    if (strcmp(command, "QUIT") == 0) {
        printf("221 Service closing control connection.\n");
        close(controlSocketFd);
        serverActive = false;
    } else if (strncmp(command, "!CWD", 4) == 0) {
        executeLocalCWD(commandBuffer);
    } else if (strncmp(command, "!PWD", 4) == 0) {
        executeLocalPWD();
    } else if (strncmp(command, "!LIST", 5) == 0) {
        executeLocalLIST();
    } else {
        if (authStatus == -1 && strncmp(command, "USER", 4) == 0) {
            char *statusCode = transmitToServer(controlSocketFd, commandBuffer);
            if (strcmp(statusCode, "331") == 0) {
                authStatus++;
            }
            free(statusCode);
        } else if (authStatus == 0 && strncmp(command, "PASS", 4) == 0) {
            char *statusCode = transmitToServer(controlSocketFd, commandBuffer);
            if (strcmp(statusCode, "230") == 0) {
                authStatus++;
            }
            free(statusCode);
        } else if (authStatus == 1) {
            if (strncmp(command, "LIST", 4) == 0) {
                if (fork() == 0) {
                    int dataChannel = initializeSocket(true, generatePort());
                    if (!configurePortCommand(controlSocketFd, dataChannel)) {
                        close(dataChannel);
                        exit(EXIT_FAILURE);
                    }

                    char *statusCode = transmitToServer(controlSocketFd, commandBuffer);
                    if (strstr(statusCode, "150") == NULL) {
                        close(dataChannel);
                        exit(EXIT_FAILURE);
                    }

                    fd_set readfds;
                    FD_ZERO(&readfds);
                    FD_SET(dataChannel, &readfds);

                    if (select(dataChannel + 1, &readfds, NULL, NULL, NULL) > 0) {
                        int clientFd = accept(dataChannel, NULL, NULL);
                        if (clientFd < 0) {
                            perror("accept failed");
                            close(dataChannel);
                            exit(EXIT_FAILURE);
                        }
                        close(dataChannel);

                        while (1) {
                            char dataBuffer[BUFFER_CAPACITY] = {0};
                            int bytesRead = recv(clientFd, dataBuffer, BUFFER_CAPACITY, 0);
                            if (bytesRead == 0) {
                                close(clientFd);
                                break;
                            }
                            printf("%s\n", dataBuffer);
                        }
                        printf("ftp> ");
                        exit(EXIT_SUCCESS);
                    }
                }
            } else if (strncmp(command, "RETR", 4) == 0) {
                if (fork() == 0) {
                    int dataChannel = initializeSocket(true, generatePort());
                    if (!configurePortCommand(controlSocketFd, dataChannel)) {
                        close(dataChannel);
                        exit(EXIT_FAILURE);
                    }

                    char *statusCode = transmitToServer(controlSocketFd, commandBuffer);
                    if (strstr(statusCode, "150") == NULL) {
                        close(dataChannel);
                        exit(EXIT_FAILURE);
                    }

                    fd_set readfds;
                    FD_ZERO(&readfds);
                    FD_SET(dataChannel, &readfds);

                    if (select(dataChannel + 1, &readfds, NULL, NULL, NULL) > 0) {
                        int clientFd = accept(dataChannel, NULL, NULL);
                        if (clientFd < 0) {
                            perror("accept failed");
                            close(dataChannel);
                            exit(EXIT_FAILURE);
                        }
                        close(dataChannel);

                        char *filename = commandBuffer + 5;
                        FILE *file = fopen(filename, "wb");
                        if (file) {
                            char fileBuffer[BUFFER_CAPACITY];
                            int totalBytes = 0;
                            int bytesRead;
                            while ((bytesRead = read(clientFd, fileBuffer, BUFFER_CAPACITY)) > 0) {
                                fwrite(fileBuffer, 1, bytesRead, file);
                                totalBytes += bytesRead;
                            }
                            printf("Total data received for %s = %d bytes.\n", filename, totalBytes);
                            fclose(file);
                        } else {
                            perror("fopen failed");
                        }
                        close(clientFd);

                        char finalBuffer[BUFFER_CAPACITY] = {0};
                        read(controlSocketFd, finalBuffer, BUFFER_CAPACITY);
                        printf("%s\n", finalBuffer);
                        printf("ftp> ");
                        exit(EXIT_SUCCESS);
                    }
                }
            } else if (strncmp(command, "STOR", 4) == 0) {
                if (fork() == 0) {
                    int dataChannel = initializeSocket(true, generatePort());
                    if (!configurePortCommand(controlSocketFd, dataChannel)) {
                        close(dataChannel);
                        exit(EXIT_FAILURE);
                    }

                    char *statusCode = transmitToServer(controlSocketFd, commandBuffer);
                    if (strstr(statusCode, "150") == NULL) {
                        close(dataChannel);
                        exit(EXIT_FAILURE);
                    }

                    fd_set readfds;
                    FD_ZERO(&readfds);
                    FD_SET(dataChannel, &readfds);

                    if (select(dataChannel + 1, &readfds, NULL, NULL, NULL) > 0) {
                        int clientFd = accept(dataChannel, NULL, NULL);
                        if (clientFd < 0) {
                            perror("accept failed");
                            close(dataChannel);
                            exit(EXIT_FAILURE);
                        }

                        char *filename = commandBuffer + 5;
                        FILE *file = fopen(filename, "rb");
                        if (file) {
                            struct stat fileStat;
                            stat(filename, &fileStat);
                            int fileSize = fileStat.st_size;

                            char *fileBuffer = malloc(fileSize);
                            fread(fileBuffer, 1, fileSize, file);

                            int bytesSent = 0;
                            while (bytesSent < fileSize) {
                                int bytes = write(clientFd, fileBuffer + bytesSent, fileSize - bytesSent);
                                if (bytes == -1) {
                                    perror("write failed");
                                    break;
                                }
                                bytesSent += bytes;
                            }
                            free(fileBuffer);
                            fclose(file);
                        } else {
                            perror("fopen failed");
                        }
                        close(clientFd);

                        char finalBuffer[BUFFER_CAPACITY] = {0};
                        read(controlSocketFd, finalBuffer, BUFFER_CAPACITY);
                        printf("%s\n", finalBuffer);
                        printf("ftp> ");
                        exit(EXIT_SUCCESS);
                    }
                }
            } else {
                transmitToServer(controlSocketFd, commandBuffer);
            }
        } else {
            transmitToServer(controlSocketFd, commandBuffer);
        }
    }
    memset(commandBuffer, 0, BUFFER_CAPACITY);
}

void executeLocalPWD() {
    char *cwd = getcwd(NULL, 0);
    if (cwd) {
        printf("Current working directory: %s\n", cwd);
        free(cwd);
    } else {
        perror("getcwd failed");
    }
}

void executeLocalCWD(char *buffer) {
    char *dir = buffer + 5;
    if (chdir(dir) != 0) {
        perror("chdir failed");
    } else {
        executeLocalPWD();
    }
}

void executeLocalLIST() {
    DIR *dir = opendir(".");
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            char type = entry->d_type == DT_DIR ? 'D' : 'F';
            printf("%c\t%s\n", type, entry->d_name);
        }
        closedir(dir);
    } else {
        perror("opendir failed");
    }
}

int generatePort() {
    srand(time(NULL));
    return 3000 + rand() % 5001;
}

char *generatePortString(const char *ip, int port) {
    int p1 = port / 256;
    int p2 = port % 256;
    char *modifiedIp = strdup(ip);

    for (int i = 0; strlen(modifiedIp) && modifiedIp[i]; i++) {
        if (modifiedIp[i] == '.') {
            modifiedIp[i] = ',';
        }
    }

    size_t messageSize = strlen(modifiedIp) + 16;
    char *message = (char *)malloc(messageSize);
    if (message != NULL) {
        snprintf(message, messageSize, "%s,%d,%d", modifiedIp, p1, p2);
    }

    free(modifiedIp);
    return message;
}

char *transmitToServer(int socketFd, const char *buffer) {
    // Clear the receive buffer
    char recvBuffer[BUFFER_CAPACITY] = {0};

    // Send the command to the server
    ssize_t sentBytes = send(socketFd, buffer, strlen(buffer), 0);
    if (sentBytes < 0) {
        perror("Error sending data to server");
        return NULL;
    }

    // Receive the server's response
    ssize_t receivedBytes = recv(socketFd, recvBuffer, BUFFER_CAPACITY - 1, 0);
    if (receivedBytes < 0) {
        perror("Error receiving data from server");
        return NULL;
    }
    recvBuffer[receivedBytes] = '\0';

    // Extract and return the status code
    char *statusCode = (char *)malloc(4);
    if (statusCode != NULL) {
        strncpy(statusCode, recvBuffer, 3);
        statusCode[3] = '\0';
    }

    // Print the server's response
    printf("%s\n", recvBuffer);

    return statusCode;
}

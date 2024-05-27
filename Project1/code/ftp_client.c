#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "ftp_common.h"

void handle_command(int client_socket, char *command);
void send_user(int client_socket, char *command);
void send_pass(int client_socket, char *command);
void send_cwd(int client_socket, char *command);
void send_retr(int client_socket, char *command);
void send_stor(int client_socket, char *command);
void send_list(int client_socket, char *command);
void send_port(int client_socket, char *command);
int setup_data_port();
int accept_data_connection(int port);

int main() {
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[1024];

    // Setup client socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(21);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    // Command loop
    while (1) {
        printf("ftp> ");
        fgets(buffer, 1024, stdin);
        buffer[strcspn(buffer, "\n")] = 0;  // Remove newline character

        if (strncmp(buffer, "QUIT", 4) == 0) {
            send(client_socket, buffer, strlen(buffer), 0);
            break;
        } else {
            handle_command(client_socket, buffer);
        }
    }

    close(client_socket);
    return 0;
}

void handle_command(int client_socket, char *command) {
    if (strncmp(command, "USER", 4) == 0) {
        send_user(client_socket, command);
    } else if (strncmp(command, "PASS", 4) == 0) {
        send_pass(client_socket, command);
    } else if (strncmp(command, "PORT", 4) == 0) {
        send_port(client_socket, command);
    } else if (strncmp(command, "CWD", 3) == 0) {
        send_cwd(client_socket, command);
    } else if (strncmp(command, "RETR", 4) == 0) {
        send_retr(client_socket, command);
    } else if (strncmp(command, "STOR", 4) == 0) {
        send_stor(client_socket, command);
    } else if (strncmp(command, "LIST", 4) == 0) {
        send_list(client_socket, command);
    } else if (command[0] == '!') {
        system(command + 1);  // Execute local shell command
    } else {
        printf("Invalid command.\n");
    }
}

void send_user(int client_socket, char *command) {
    send(client_socket, command, strlen(command), 0);
    char response[1024];
    recv(client_socket, response, 1024, 0);
    printf("%s", response);
}

void send_pass(int client_socket, char *command) {
    send(client_socket, command, strlen(command), 0);
    char response[1024];
    recv(client_socket, response, 1024, 0);
    printf("%s", response);
}

void send_cwd(int client_socket, char *command) {
    send(client_socket, command, strlen(command), 0);
    char response[1024];
    recv(client_socket, response, 1024, 0);
    printf("%s", response);
}

void send_retr(int client_socket, char *command) {
    // Send PORT command first
    int port = setup_data_port();
    char port_command[1024];
    sprintf(port_command, "PORT 127,0,0,1,%d,%d\r\n", port / 256, port % 256);
    send(client_socket, port_command, strlen(port_command), 0);
    char response[1024];
    recv(client_socket, response, 1024, 0);
    printf("%s", response);

    // Send RETR command
    send(client_socket, command, strlen(command), 0);
    recv(client_socket, response, 1024, 0);
    printf("%s", response);

    // Receive file data
    int data_socket = accept_data_connection(port);
    FILE *file = fopen("downloaded_file.txt", "wb");
    if (file) {
        char data[1024];
        int bytes_read;
        while ((bytes_read = recv(data_socket, data, 1024, 0)) > 0) {
            fwrite(data, 1, bytes_read, file);
        }
        fclose(file);
    }
    close(data_socket);
}

void send_stor(int client_socket, char *command) {
    // Send PORT command first
    int port = setup_data_port();
    char port_command[1024];
    sprintf(port_command, "PORT 127,0,0,1,%d,%d\r\n", port / 256, port % 256);
    send(client_socket, port_command, strlen(port_command), 0);
    char response[1024];
    recv(client_socket, response, 1024, 0);
    printf("%s", response);

    // Send STOR command
    send(client_socket, command, strlen(command), 0);
    recv(client_socket, response, 1024, 0);
    printf("%s", response);

    // Send file data
    int data_socket = accept_data_connection(port);
    FILE *file = fopen("upload_file.txt", "rb");
    if (file) {
        char data[1024];
        int bytes_read;
        while ((bytes_read = fread(data, 1, 1024, file)) > 0) {
            send(data_socket, data, bytes_read, 0);
        }
        fclose(file);
    }
    close(data_socket);
}

void send_list(int client_socket, char *command) {
    // Send PORT command first
    int port = setup_data_port();
    char port_command[1024];
    sprintf(port_command, "PORT 127,0,0,1,%d,%d\r\n", port / 256, port % 256);
    send(client_socket, port_command, strlen(port_command), 0);
    char response[1024];
    recv(client_socket, response, 1024, 0);
    printf("%s", response);

    // Send LIST command
    send(client_socket, command, strlen(command), 0);
    recv(client_socket, response, 1024, 0);
    printf("%s", response);

    // Receive directory listing
    int data_socket = accept_data_connection(port);
    char data[1024];
    int bytes_read;
    while ((bytes_read = recv(data_socket, data, 1024, 0)) > 0) {
        printf("%s", data);
    }
    close(data_socket);
}

void send_port(int client_socket, char *command) {
    send(client_socket, command, strlen(command), 0);
    char response[1024];
    recv(client_socket, response, 1024, 0);
    printf("%s", response);
}

int setup_data_port() {
    int data_socket, port;
    struct sockaddr_in data_addr;

    data_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (data_socket < 0) {
        perror("Data socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&data_addr, 0, sizeof(data_addr));
    data_addr.sin_family = AF_INET;
    data_addr.sin_addr.s_addr = INADDR_ANY;
    data_addr.sin_port = 0;

    if (bind(data_socket, (struct sockaddr*)&data_addr, sizeof(data_addr)) < 0) {
        perror("Bind failed");
        close(data_socket);
        exit(EXIT_FAILURE);
    }

    socklen_t addr_len = sizeof(data_addr);
    if (getsockname(data_socket, (struct sockaddr*)&data_addr, &addr_len) < 0) {
        perror("Get socket name failed");
        close(data_socket);
        exit(EXIT_FAILURE);
    }

    port = ntohs(data_addr.sin_port);
    if (listen(data_socket, 1) < 0) {
        perror("Listen failed");
        close(data_socket);
        exit(EXIT_FAILURE);
    }

    return port;
}

int accept_data_connection(int port) {
    int data_socket, new_socket;
    struct sockaddr_in data_addr;
    socklen_t addr_len = sizeof(data_addr);

    data_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (data_socket < 0) {
        perror("Data socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&data_addr, 0, sizeof(data_addr));
    data_addr.sin_family = AF_INET;
    data_addr.sin_port = htons(port);
    data_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(data_socket, (struct sockaddr*)&data_addr, sizeof(data_addr)) < 0) {
        perror("Bind failed");
        close(data_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(data_socket, 1) < 0) {
        perror("Listen failed");
        close(data_socket);
        exit(EXIT_FAILURE);
    }

    new_socket = accept(data_socket, (struct sockaddr*)&data_addr, &addr_len);
    if (new_socket < 0) {
        perror("Accept failed");
        close(data_socket);
        exit(EXIT_FAILURE);
    }

    close(data_socket);
    return new_socket;
}

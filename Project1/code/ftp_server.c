#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/stat.h>


void handle_client(int client_socket);
void handle_user(int client_socket, char *command);
void handle_pass(int client_socket, char *command);
void handle_cwd(int client_socket, char *command);
void handle_retr(int client_socket, char *command);
void handle_stor(int client_socket, char *command);
void handle_list(int client_socket, char *command);
void handle_port(int client_socket, char *command);
void handle_pwd(int client_socket);
void handle_quit(int client_socket);
int setup_data_connection();

char client_ip[16] = "";
int client_data_port = 0;
char logged_in_user[50] = "";
char logged_in_pass[50] = "";

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Setup server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(2121);  // Using non-privileged port 2121

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 5) < 0) {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server started successfully.\n");

    // Use select() for handling multiple connections
    fd_set read_fds;
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(server_socket, &read_fds);
        if (select(server_socket + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("Select failed");
            close(server_socket);
            exit(EXIT_FAILURE);
        }
        if (FD_ISSET(server_socket, &read_fds)) {
            client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
            if (client_socket < 0) {
                perror("Accept failed");
                continue;
            }
            if (fork() == 0) {
                close(server_socket);
                handle_client(client_socket);
                close(client_socket);
                exit(0);
            }
            close(client_socket);
        }
    }
    return 0;
}

void handle_client(int client_socket) {
    char buffer[1024];
    int n;

    // Send welcome message
    send(client_socket, "220 Service ready for new user.\r\n", 32, 0);

    // Command loop
    while ((n = recv(client_socket, buffer, 1024, 0)) > 0) {
        buffer[n] = '\0';
        if (strncmp(buffer, "USER", 4) == 0) {
            handle_user(client_socket, buffer);
        } else if (strncmp(buffer, "PASS", 4) == 0) {
            handle_pass(client_socket, buffer);
        } else if (strncmp(buffer, "PORT", 4) == 0) {
            handle_port(client_socket, buffer);
        } else if (strncmp(buffer, "CWD", 3) == 0) {
            handle_cwd(client_socket, buffer);
        } else if (strncmp(buffer, "RETR", 4) == 0) {
            handle_retr(client_socket, buffer);
        } else if (strncmp(buffer, "STOR", 4) == 0) {
            handle_stor(client_socket, buffer);
        } else if (strncmp(buffer, "LIST", 4) == 0) {
            handle_list(client_socket, buffer);
        } else if (strncmp(buffer, "PWD", 3) == 0) {
            handle_pwd(client_socket);
        } else if (strncmp(buffer, "QUIT", 4) == 0) {
            handle_quit(client_socket);
            break;
        } else {
            send(client_socket, "502 Command not implemented.\r\n", 30, 0);
        }
    }
}


void print_current_directory() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Current working directory: %s\n", cwd);
    } else {
        perror("getcwd() error");
    }
}


void handle_user(int client_socket, char *command) {
    char username[50];
    sscanf(command, "USER %s", username);

     print_current_directory();

    // Use relative path to users.csv
    FILE *fp = fopen("../users.csv", "r");
    if (!fp) {
        perror("Failed to open users.csv");
        send(client_socket, "530 Not logged in.\r\n", 21, 0);
        return;
    }

    char file_user[50], file_pass[50];
    int user_found = 0;
    while (fscanf(fp, "%49[^,],%49[^\n]\n", file_user, file_pass) != EOF) {
        if (strcmp(username, file_user) == 0) {
            user_found = 1;
            strcpy(logged_in_user, username);
            strcpy(logged_in_pass, file_pass);
            break;
        }
    }
    fclose(fp);

    if (user_found) {
        send(client_socket, "331 Username OK, need password.\r\n", 33, 0);
    } else {
        send(client_socket, "530 Not logged in.\r\n", 21, 0);
    }
}

void handle_pass(int client_socket, char *command) {
    char password[50];
    sscanf(command, "PASS %s", password);

    if (strlen(logged_in_user) == 0) {
        send(client_socket, "530 Not logged in.\r\n", 21, 0);
        return;
    }

    if (strcmp(password, logged_in_pass) == 0) {
        struct stat st = {0};
        char user_dir[100];
        snprintf(user_dir, sizeof(user_dir), "../server/%s", logged_in_user);
        if (stat(user_dir, &st) == -1) {
            if (mkdir(user_dir, 0700) == -1) {
                perror("Failed to create user directory");
                send(client_socket, "550 Failed to create user directory.\r\n", 39, 0);
                return;
            }
        }
        if (chdir(user_dir) == -1) {
            perror("Failed to change to user directory");
            send(client_socket, "550 Failed to change to user directory.\r\n", 43, 0);
            return;
        }
        send(client_socket, "230 User logged in, proceed.\r\n", 30, 0);
    } else {
        send(client_socket, "530 Not logged in.\r\n", 21, 0);
        logged_in_user[0] = '\0';
        logged_in_pass[0] = '\0';
    }
}

void handle_cwd(int client_socket, char *command) {
    char path[1024];
    sscanf(command, "CWD %s", path);
    if (chdir(path) == 0) {
        send(client_socket, "200 Directory changed.\r\n", 24, 0);
    } else {
        send(client_socket, "550 Failed to change directory.\r\n", 33, 0);
    }
}

void handle_retr(int client_socket, char *command) {
    char filename[1024];
    sscanf(command, "RETR %s", filename);
    int data_socket = setup_data_connection();
    if (data_socket < 0) {
        send(client_socket, "425 Can't open data connection.\r\n", 33, 0);
        return;
    }
    send(client_socket, "150 File status okay; about to open data connection.\r\n", 53, 0);

    FILE *file = fopen(filename, "rb");
    if (file) {
        char data[1024];
        int bytes_read;
        while ((bytes_read = fread(data, 1, 1024, file)) > 0) {
            send(data_socket, data, bytes_read, 0);
        }
        fclose(file);
        send(client_socket, "226 Transfer complete.\r\n", 26, 0);
    } else {
        send(client_socket, "550 File not found.\r\n", 21, 0);
    }
    close(data_socket);
}

void handle_stor(int client_socket, char *command) {
    char filename[1024];
    sscanf(command, "STOR %s", filename);
    int data_socket = setup_data_connection();
    if (data_socket < 0) {
        send(client_socket, "425 Can't open data connection.\r\n", 33, 0);
        return;
    }
    send(client_socket, "150 File status okay; about to open data connection.\r\n", 53, 0);

    FILE *file = fopen(filename, "wb");
    if (file) {
        char data[1024];
        int bytes_read;
        while ((bytes_read = recv(data_socket, data, 1024, 0)) > 0) {
            fwrite(data, 1, bytes_read, file);
        }
        fclose(file);
        send(client_socket, "226 Transfer complete.\r\n", 26, 0);
    } else {
        send(client_socket, "550 Failed to create file.\r\n", 28, 0);
    }
    close(data_socket);
}

void handle_list(int client_socket, char *command) {
    if (strlen(client_ip) == 0 || client_data_port == 0) {
        send(client_socket, "503 Bad sequence of commands.\r\n", 31, 0);
        return;
    }

    int data_socket = setup_data_connection();
    if (data_socket < 0) {
        send(client_socket, "425 Can't open data connection.\r\n", 33, 0);
        return;
    }

    send(client_socket, "150 File status okay; about to open data connection.\r\n", 53, 0);

    FILE *pipe = popen("ls", "r");
    if (pipe) {
        char data[1024];
        while (fgets(data, sizeof(data), pipe)) {
            send(data_socket, data, strlen(data), 0);
        }
        pclose(pipe);
        send(client_socket, "226 Transfer complete.\r\n", 26, 0);
    } else {
        send(client_socket, "550 Failed to list directory.\r\n", 31, 0);
    }
    close(data_socket);
}

void handle_port(int client_socket, char *command) {
    int h1, h2, h3, h4, p1, p2;
    sscanf(command, "PORT %d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2);
    sprintf(client_ip, "%d.%d.%d.%d", h1, h2, h3, h4);
    client_data_port = (p1 * 256) + p2;
    send(client_socket, "200 PORT command successful.\r\n", 30, 0);
}

void handle_pwd(int client_socket) {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        char response[1050];
        snprintf(response, sizeof(response), "257 \"%s\"\r\n", cwd);
        send(client_socket, response, strlen(response), 0);
    } else {
        send(client_socket, "550 Failed to get current directory.\r\n", 38, 0);
    }
}

void handle_quit(int client_socket) {
    send(client_socket, "221 Service closing control connection.\r\n", 40, 0);
}

int setup_data_connection() {
    int data_socket;
    struct sockaddr_in data_addr;

    data_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (data_socket < 0) {
        perror("Data socket creation failed");
        return -1;
    }

    memset(&data_addr, 0, sizeof(data_addr));
    data_addr.sin_family = AF_INET;
    data_addr.sin_port = htons(client_data_port);
    inet_pton(AF_INET, client_ip, &data_addr.sin_addr);

    if (connect(data_socket, (struct sockaddr*)&data_addr, sizeof(data_addr)) < 0) {
        perror("Data connection failed");
        close(data_socket);
        return -1;
    }

    return data_socket;
}

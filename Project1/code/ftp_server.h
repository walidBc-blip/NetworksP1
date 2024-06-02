#ifndef FTP_SERVER_H
#define FTP_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <ctype.h> 
#include <errno.h>

// Function declarations
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
void print_welcome_message(int client_socket);
int setup_data_connection();
void trim_whitespace(char *str); 
// Global variables
extern char client_ip[16];
extern int client_data_port;
extern char logged_in_user[50]; // Store the logged-in user's name
extern char logged_in_pass[50]; // Store the logged-in user's password

#endif // FTP_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

#define BUFFER_SIZE 1024

void prepare_tcpClient(struct addrinfo *cli_hints, struct addrinfo *cli_res, int* cli_fd);
void prepare_tcpServer(struct addrinfo *ser_hints, struct addrinfo *ser_res, int *ser_listenfd);

void print_TCParray();

bool validate_inputArgs(int argc, char* argv[]);

void tcp_cli ();

int choose_extern();

void tcp_serv ();

 
 
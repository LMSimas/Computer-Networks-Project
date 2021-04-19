#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#define BUFFER_SIZE 1024
void print_TCParray(char[5][BUFFER_SIZE], char[5][BUFFER_SIZE], int);

bool validate_inputArgs(int argc, char* argv[]);

void tcp_cli (char IP_Array[5][BUFFER_SIZE], char Port_Array[5][BUFFER_SIZE], int array_size);

int choose_extern(char IP_Array[5][BUFFER_SIZE], char Port_Array[5][BUFFER_SIZE], int array_size);

void tcp_serv (char IP_Array[5][BUFFER_SIZE], char Port_Array[5][BUFFER_SIZE]);


#include "headerFile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#define BUFFER_SIZE 1024





int main (int argc, char* argv[]){

    char buffer[BUFFER_SIZE], command[BUFFER_SIZE], netid[BUFFER_SIZE], nodeid[BUFFER_SIZE], message[BUFFER_SIZE];  //message para o registo de um novo no
    
    enum {notreg, goingout, regwait, reg, notregwait} state;
    fd_set rfds;
    int maxfd, counter;

    struct addrinfo hints,*res; //for udp server comms
    int fd,errcode;             //for udp server comms
    ssize_t n;
    struct sockaddr server_addr;
    socklen_t addrlen;

    /*VALIDATION STEP*/
    if(!validate_inputArgs(argc, argv)) // if NOT validated
        exit(1);//in order 2 exit the programm

    /*APPLICATION STARTS HERE*/


    sockfd=socket(AF_INET,SOCK_DGRAM,0);//UDP socket 
    if(sockfd==-1)
    {
        printf("Error: Unexpected socket\n");
        exit(1);
    }
    memset(&hints,0,sizeof hints); 
    hints.ai_family=AF_INET; //IPv4 
    hints.ai_socktype=SOCK_DGRAM;//UDP socket
    errcode = getaddrinfo(argv[3],argv[4],&hints,&server_info); 
    if(errcode!=0)
    {
        printf("Error: Unexpected getaddrinfo\n");
        exit(1);
    }

    //MAQUINA DE ESTADOS 

    printf("ndn> "); fflush(stdout); //prompt
    state = notreg;
    while(state!=goingout)
    {
        FD_ZERO(&rfds);
        switch (state) //numero de descritores a monitorizar depende do estado
        { 
            case notreg:     FD_SET(0, &rfds); maxfd=0; break;
            case regwait:    FD_SET(0, &rfds); FD_SET(sockfd, &rfds); maxfd=sockfd; break;
            case reg:        FD_SET(0, &rfds); maxfd=0; break;
            case notregwait: FD_SET(0, &rfds); FD_SET(sockfd, &rfds); maxfd=sockfd; break;
        }//switch(state)

        counter=select(maxfd+1,&rfds,(fd_set*)NULL,(fd_set*)NULL,(struct timeval *)NULL);
        if(counter<=0){printf("Error: Unexpected (select)\n");}exit(1);

        for (; counter ; --counter) //corre tantas vezes quantas o retorno do select
            switch (state)
            {
            case notreg:     //---------------------------------------------------------------------------
                if(FD_ISSET(0,&rfds)) //stdin
                {
                    FD_CLR(0,&rfds);
                    if (fgets(buffer, BUFFER_SIZE, stdin)!=NULL) 
                    {
                        if(sscanf(buffer, "%s", command)==1)
                        {
                            if(strcmp(command, "join")==0) 
                            {
                                printf("Command: join \n");
                                if(sscanf(buffer, "%*s%s%s", netid, nodeid)!=2)
                                    printf("Error: missing arguments\n");
                                else
                                {
                                    //get nodes list
                                    //select one node
                                    //connect to it
                                    //register -- done
                                    sprintf(message, "REG %s %s %s", netid, argv[1], argv[2]);
                                    n=sendto(sockfd,message, strlen(message), 0, server_info->ai_addr, ,server_info->ai_addrlen); 
                                    if(n==-1)
                                    {
                                        printf("Error: Unexpected sendto\n");
                                        exit(1);
                                    }
                                    state=regwait;

                                }


                                //operacoes necessarias
                            }
                            else if (strcmp(command, "leave")==0) printf("Makes no sense to leave before joining \n");
                            else if (strcmp(command, "exit")==0) {printf("Going out.\n\n"); state=goingout;}
                            //resto dos comandos possiveis
                            //...
                            else printf("Error: Unknown command. \n");
                        }//if command

                    }

                if(state!=goingout)printf("ndn> "); fflush(stdout); //prompt
                }//stdin
            break;//notreg

            case regwait:    //---------------------------------------------------------------------------
                if(FD_ISSET(0,&rfds)) //stdin
                {
                    FD_CLR(0,&rfds);
                    if (fgets(buffer, BUFFER_SIZE, stdin)!=NULL) 
                    {
                        if(sscanf(buffer, "%s", command)==1)
                        {
                            if(strcmp(command, "join")==0) printf("Join in progress... \n");
                            else if (strcmp(command, "leave")==0) printf("Makes no sense to leave before joining \n");
                            else if (strcmp(command, "exit")==0) {printf("Going out.\n\n"); state=goingout;}
                            //resto dos comandos possiveis
                            //...
                            else printf("Error: Unknown command. \n");
                        }//if commands
                    }
                    if(state!=goingout)printf("ndn> "); fflush(stdout); //prompt
                }//stdin
                else if (FD_ISSET(sockfd,&rfds)) 
                {
                    FD_CLR(sockfd,&rfds);
                    n=recvfrom(sockfd, message, BUFFER_SIZE-1, 0, &server_addr, &addrlen); 
                    if(n==-1)
                    {
                        printf("Error: Unexpected recvfrom\n");
                        exit(1);
                    }
                    message[n]='\0';
                    if (strcmp(message, "OKREG")==0){printf("Registered.\nndn>"); fflush(stdout); state=reg;} //esta registado


                }//UDP socket
            break;//regwait
            
            case reg:        //---------------------------------------------------------------------------
                if(FD_ISSET(0,&rfds)) //stdin
                {
                    FD_CLR(0,&rfds);
                    if (fgets(buffer, BUFFER_SIZE, stdin)!=NULL) 
                    {
                        if(sscanf(buffer, "%s", command)==1)
                        {
                            if(strcmp(command, "join")==0) printf("Already joined. \n");
                            else if (strcmp(command, "leave")==0) 
                            {
                                //muita coisa para fazer aqui
                                //...
                                sprintf(message, "UNREG %s %s %s", netid, argv[1], argv[2]);
                                n=sendto(sockfd,message, strlen(message), 0, server_info->ai_addr, ,server_info->ai_addrlen); 
                                if(n==-1)
                                {
                                    printf("Error: Unexpected sendto\n");
                                    exit(1);
                                }
                                state=notregwait;
                            }
                            else if (strcmp(command, "exit")==0) {printf("Going out.\n\n"); state=goingout;}
                            //resto dos comandos possiveis
                            //...
                            else printf("Error: Unknown command. \n");
                            
                        }//if command

                    }
                if(state!=goingout)printf("ndn> "); fflush(stdout); //prompt
                }//stdin
            break;//reg

            case notregwait: //---------------------------------------------------------------------------
                if(FD_ISSET(0,&rfds)) //stdin
                {
                    FD_CLR(0,&rfds);
                    if (fgets(buffer, BUFFER_SIZE, stdin)!=NULL) 
                    {
                        if(sscanf(buffer, "%s", command)==1)
                        {
                            if(strcmp(command, "join")==0) printf("Leave in progress... \n");
                            else if (strcmp(command, "leave")==0) printf("Leave in progress... \n");
                            else if (strcmp(command, "exit")==0) {printf("Going out.\n\n"); state=goingout;}
                            //resto dos comandos possiveis
                            //...
                            else printf("Error: Unknown command. \n");
                        }//if commands
                    }
                    if(state!=goingout)printf("ndn> "); fflush(stdout); //prompt
                }//stdin
                else if (FD_ISSET(sockfd, &rfds)) 
                {
                    FD_CLR(sockfd,&rfds);
                    n=recvfrom(sockfd, message, BUFFER_SIZE-1, 0, &server_addr, &addrlen); 
                    if(n==-1)
                    {
                        printf("Error: Unexpected recvfrom\n");
                        exit(1);
                    }
                    message[n]='\0';
                    if (strcmp(message, "OKUNREG")==0){printf("Unregistered.\nndn>"); fflush(stdout); state=notreg;} 
                }//UDP socket
            break;//unregwait

            }//switch(state)
        
    }//while()

close(sockfd);
freeaddrinfo(server_info);

exit(0);
}//main


bool validate_inputArgs(int argc, char* argv[]){
    unsigned int adr1,adr2,adr3,adr4; //input address containers
    unsigned int port;
    char token[128];

    /*CHECK NUMBER OF ARGS*/
    if(argc != 5){
        printf("ERROR: Please insert a valid input argument list\n\nUsage: ndn <ipaddr> <tcpport> <reg_ipaddr> <reg_udpport>\n");
        return false;
    }

    /*CHECK IF THE TCP IP IS VALID*/
    if(sscanf(argv[1], "%u.%u.%u.%u%s", &adr1, &adr2, &adr3, &adr4, token)!=4){//returns the number of variables filled.
        printf("ERROR: Bad <ipaddr> -- That IP doesn't have a correct format\n\nUsage: ndn <ipaddr> <tcpport> <reg_ipaddr> <reg_udpport>\n");
        return false;
    }
    else if( adr1>255 || adr2>255 || adr3>255 || adr4>255){//if 1 address > 255 (not a valid one)
        printf("ERROR: Bad <ipaddr> -- An IP address should not have a value over 255\n\nUsage: ndn <ipaddr> <tcpport> <reg_ipaddr> <reg_udpport>\n");
        return false;
    }
    /*CHECK IF THE TCP PORT IS VALID*/
    if(sscanf(argv[2], "%u", &port)!=1 || port > 65535){
        printf("ERROR: Bad <tcpport> -- Port should not have a value over 65535\n\nUsage: ndn <ipaddr> <tcpport> <reg_ipaddr> <reg_udpport>\n");
        return false;
    }


    /*CHECK IF THE UDP IP IS VALID*/
    if(sscanf(argv[3], "%u.%u.%u.%u%s", &adr1, &adr2, &adr3, &adr4, token)!=4){
        printf("ERROR: Bad <reg_ipaddr> -- That IP doesn't have a correct format\n\nUsage: ndn <ipaddr> <tcpport> <reg_ipaddr> <reg_udpport>\n");
        return false;
    }
    else if( adr1>255 || adr2>255 || adr3>255 || adr4>255){//if 1 address > 255 (not a valid one)
        printf("ERROR: Bad <reg_ipaddr> -- An IP address should not have a value over 255\n\nUsage: ndn <ipaddr> <tcpport> <reg_ipaddr> <reg_udpport>\n");
        return false;
    }
    /*CHECK IF THE UDP PORT IS VALID*/
    if(sscanf(argv[4], "%u", &port)!=1 || port > 65535){
        printf("ERROR: Bad <reg_udpport> -- Port should not have a value over 65535\n\nUsage: ndn <ipaddr> <tcpport> <reg_ipaddr> <reg_udpport>\n");
        return false;
    }
        
    printf("all validated, let's keep going\n\n");
    return true; //if nothing's wrong, return 1 == all validated
}



void execute();
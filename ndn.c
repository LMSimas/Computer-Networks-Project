#include "headerFile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>



typedef struct _node {
    char netid [BUFFER_SIZE];
    
    char nodeid [BUFFER_SIZE];
    char nodeIP[BUFFER_SIZE];
    char nodeTCP[BUFFER_SIZE];

    char extern_id [BUFFER_SIZE];
    char extern_IP [BUFFER_SIZE];
    char extern_PORT [BUFFER_SIZE];

    char backup_id [BUFFER_SIZE];
    char backup_IP [BUFFER_SIZE];
    char backup_PORT [BUFFER_SIZE];
} node_s;


node_s node;
int flag_list = 0;
char TCP_IParray[5][BUFFER_SIZE];//5 for now
char TCP_PORTarray[5][BUFFER_SIZE];
int TCP_NodesCounter = 0;
char tcp_msg[BUFFER_SIZE];

int main (int argc, char* argv[]){

    char buffer[BUFFER_SIZE], command[BUFFER_SIZE], netid[BUFFER_SIZE], nodeid[BUFFER_SIZE], message[BUFFER_SIZE], message1[BUFFER_SIZE];  //message para o registo de um novo no
    char *token;

    enum {notreg, goingout, regwait, reg, notregwait, listwait} state;
    fd_set rfds;
    int maxfd, counter, sockfd;


    struct addrinfo hints, *server_info; //for udp server comms
    int fd,errcode;                      //for udp server comms
    ssize_t n;
    struct sockaddr server_addr;
    socklen_t addrlen;

    /*TCP CLIENT VARIABLES*/
    struct addrinfo cli_hints;
    struct addrinfo *cli_res;
    int cli_fd;
    /*TCP SERVER VARIABLES*/
    struct addrinfo ser_hints;
    struct addrinfo *ser_res;
    int ser_listenfd;
    int ser_acceptfd;
    int myclient_fd;
    struct sockaddr myclient_addr;

    /*VARIAVEIS DESCARTÁVEIS*/
    char return_message[BUFFER_SIZE];
    /*****************************/
    /*****PROGRAM STARTS HERE*****/
    /*****************************/

    /*VALIDATION STEP*/
    if(!validate_inputArgs(argc, argv)) // if NOT validated
        exit(1);//in order 2 exit the programm

    /*APPLICATION STARTS HERE*/
    strcpy(node.nodeIP, argv[1]);
    strcpy(node.nodeTCP, argv[2]);

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
            case reg:        FD_SET(0, &rfds); FD_SET(cli_fd, &rfds); FD_SET(ser_listenfd, &rfds); maxfd=ser_listenfd;
            if(myclient_fd!=0){//if NO CLIENTS CONNECTED
                FD_SET(myclient_fd, &rfds); maxfd=myclient_fd;
            }
             break;//nsure about cli_fd
            case notregwait: FD_SET(0, &rfds); FD_SET(sockfd, &rfds); maxfd=sockfd; break;
            case listwait:   FD_SET(0, &rfds); FD_SET(sockfd, &rfds); maxfd=sockfd; break;
        }//switch(state)

        if (state == notreg && flag_list) //tem que percorrer o notreg (pelo menos) uma vez sem qq input de descritores quando ja tem a lsita de noz
        {
            counter = 1;
        }
        else 
        {
            counter = select(maxfd+1,&rfds,(fd_set*)NULL,(fd_set*)NULL,(struct timeval *)NULL);
        }

        if(counter<=0)
        {
            printf("Error: Unexpected (select)\n");
            exit(1);
        }

        for (; counter ; --counter) //corre tantas vezes quantas o retorno do select
            switch (state)
            {

            case notreg:     //---------------------------------------------------------------------------
                //printf("ENTRA NO NOTREG \n");
                if(FD_ISSET(0,&rfds) && !flag_list) //stdin
                {
                    //printf("ENTRA NO ISSET \n");

                    FD_CLR(0,&rfds);
                    if (fgets(buffer, BUFFER_SIZE, stdin)!=NULL) 
                    {
                        //printf("entrou no fgets\n"); 
                        if(sscanf(buffer, "%s", command)==1)
                        {
                            if(strcmp(command, "join")==0) 
                            {
                                printf("Command: join \n");
                                if(sscanf(buffer, "%*s%s%s", node.netid, node.nodeid)!=2)
                                    printf("Error: missing arguments\n");
                                else if(!flag_list){ 
                                    //new
                                    sprintf(message, "NODES %s", node.netid);
                                    n=sendto(sockfd,message, strlen(message), 0, server_info->ai_addr, server_info->ai_addrlen); 
                                    if(n==-1)
                                    {
                                        printf("Error: Unexpected sendto\n");
                                        exit(1);
                                    }
                                    state=listwait;
                                }

                                //resto das operacoes necessarias
                            }
                            else if (strcmp(command, "leave")==0) printf("Makes no sense to leave before joining \n");
                            else if (strcmp(command, "exit")==0) {printf("Going out.\n\n"); state=goingout;}
                            //resto dos comandos possiveis
                            //...
                            else printf("Error: Unknown command. \n");
                        }//if command                    
                    }//fgets

                    if(state!=goingout && flag_list==0)printf("ndn> "); fflush(stdout); //prompt
                }//stdin
                if (flag_list) {     //segunda vez que entra neste estado ja c a lista

                    //select one node
                    //connect to it
                    //tcp_cli();
                    prepare_tcpClient(&cli_hints, cli_res, &cli_fd);//CLIENT PREPARED

                    //register -- done
                    
                    sprintf(message, "REG %s %s %s", node.netid, node.nodeIP, node.nodeTCP);
                    n=sendto(sockfd,message, strlen(message), 0, server_info->ai_addr, server_info->ai_addrlen); 
                    if(n==-1)
                    {
                        printf("Error: Unexpected sendto\n");
                        exit(1);
                    }
                    flag_list = 0;
                    state=regwait;                  
                }//else if flaglist

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
                    if (strcmp(message, "OKREG")==0){
                        printf("Registered.\nndn>");
                        fflush(stdout);
                        prepare_tcpServer(&ser_hints, ser_res, &ser_listenfd);//SERVER PREPARED
                        state=reg;
                        } //esta registado

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
                                sprintf(message, "UNREG %s %s %s", node.netid, argv[1], argv[2]);
                                n=sendto(sockfd,message, strlen(message), 0, server_info->ai_addr, server_info->ai_addrlen); 
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
                /*******SERVER recebe NEW CLIENT******/
                else if (FD_ISSET(ser_listenfd,&rfds)){
                    FD_CLR(ser_listenfd,&rfds);
                    //accept
                    addrlen=sizeof(myclient_addr); 
                        if((myclient_fd=accept(ser_listenfd, &myclient_addr, &addrlen))==-1)//accept the new client
                        {
                            printf("Error: Unexpected accept\n");
                            exit(1);
                        }
                        else{
                            read(myclient_fd, tcp_msg, BUFFER_SIZE);
                            printf("Msg from Client\n :: %s\n", tcp_msg);

                            sprintf(tcp_msg, "EXTERN %s %s\n", node.backup_IP, node.backup_PORT); //EXTERN IP TCP<LF>
                            write(myclient_fd, tcp_msg, strlen(tcp_msg));
                        }

                }
                /*******CLIENT a receber do SERVER*******/
                else if (FD_ISSET(cli_fd,&rfds)){ 
                    FD_CLR(cli_fd,&rfds);
                    //print resposta do cliente
                    n=read(cli_fd,return_message,BUFFER_SIZE);
                    if(n==-1){
                        printf("Error rcving the server message\n\n");
                        exit(1);
                    }
                    else{//tudo ok
                        printf("Server Message:\n %s", return_message);
                    }
                }
                /**********Server recebe dos Client*******/
                else if(FD_ISSET(myclient_fd, &rfds)){//FZR FOR PARA O VECTOR DE CLIENTS
                    FD_CLR(myclient_fd, &rfds);
                    n=read(myclient_fd,return_message,BUFFER_SIZE);
                    if(n==-1){
                        printf("Error rcving the server message\n\n");
                        exit(1);
                    }
                    else{//tudo ok
                        printf("Client Message\n %s", return_message);
                    }
                }
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

            case listwait:    //---------------------------------------------------------------------------
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
                    //printf("mensagem -> %s\n", message);

                    if (sscanf(message, "%s", message1)==1)
                    { 
                        //printf("mensagem comeca com -> %s\n", message1);
                        if (strcmp(message1, "NODESLIST")==0)
                        {   
                            token = strtok(message, "\n");//get the "NODESLIST\n"

                            while(token != NULL && TCP_NodesCounter<5){
                                //printf("%s\n\n", token);

                                token = strtok(NULL, " ");//get TCP IP
                                if(token != NULL){//if we don't have any node in the list
                                    sscanf(token, "%s", TCP_IParray[TCP_NodesCounter]);
                                    token = strtok(NULL, "\n");//get TCP PORT
                                    sscanf(token, "%s", TCP_PORTarray[TCP_NodesCounter]);
                                    //here a new node is already in the array
                                    TCP_NodesCounter++;
                                }

                            }
                                
                            
                            printf("List Received.\nndn>"); 
                            print_TCParray();
                            fflush(stdout);
                        } //lista recebida
                        else printf("Error: Return message descriptor . \n");
                    }
                    else printf("Error: Return message . \n");

                    flag_list = 1;
                    //printf("VAI PARA O NOTREG \n");
                    state=notreg;
                }//UDP socket
            break;//listwait

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


void prepare_tcpClient(struct addrinfo *cli_hints, struct addrinfo *cli_res, int* cli_fd){
    if(choose_extern()!=1)  //se for o primeiro no a ser registado salta 
    {
        //struct addrinfo hints, *res;
        int n;
        int var = 4;
        //ssize_t nbytes, nleft, nwritten, nread; 
        //char *ptr,buffer[128+1];

        *cli_fd = socket(AF_INET,SOCK_STREAM,0);//TCP socket 
        if(*cli_fd==-1)
        {
            printf("Error: Unexpected TCP_cli socket\n");
            exit(1); 
        }
        memset(cli_hints,0,sizeof (*cli_hints)); 
        cli_hints->ai_family=AF_INET;//IPv4
        cli_hints->ai_socktype=SOCK_STREAM;//TCP socket

        printf("Node IP/PORT%s %s\n\n", node.extern_IP, node.extern_PORT);
        n=getaddrinfo(node.extern_IP, node.extern_PORT, cli_hints, &cli_res);       //meter aqui o TCP do no escolhido 
        if(n!=0)
        {
            printf("Error: Unexpected getaddrinfo cli\n");
            exit(1);
        }
        //printf("cli_res->ai_addr %s\n", cli_res->ai_addr);
        n=connect(*cli_fd,cli_res->ai_addr,cli_res->ai_addrlen); 
        var = errno;
        printf("errono:: %d\n", var);
        if(n==-1)
        {
            printf("Error: Unexpected connect\n");
            //var = h_errno;
            printf("ERRNO: %d\n", var);
            exit(1);
        }
        else{
            printf("client successfully connected\n\n");
            sprintf(tcp_msg, "NEW %s %s\n", node.nodeIP, node.nodeTCP); //NEW IP TCP <LF>
            write(*cli_fd, tcp_msg, strlen(tcp_msg));

            read(*cli_fd, tcp_msg, BUFFER_SIZE);
            printf("ConnectingMSG from Server\n::%s\n", tcp_msg);
        }
    }
    return;
}
void tcp_cli ()
{
    if(choose_extern()!=1)  //se for o primeiro no a ser registado salta 
    {
        struct addrinfo hints, *res;
        int fd, n;
        ssize_t nbytes, nleft, nwritten, nread; 
        char *ptr,buffer[128+1];

        fd = socket(AF_INET,SOCK_STREAM,0);//TCP socket 
        if(fd==-1)
        {
            printf("Error: Unexpected TCP_cli socket\n");
            exit(1); 
        }
        memset(&hints,0,sizeof hints); 
        hints.ai_family=AF_INET;//IPv4
        hints.ai_socktype=SOCK_STREAM;//TCP socket

        
        n=getaddrinfo(node.extern_IP, node.extern_PORT, &hints,&res);       //meter aqui o TCP do no escolhido 
        if(n!=0)
        {
            printf("Error: Unexpected getaddrinfo cli\n");
            exit(1);
        }
        n=connect(fd,res->ai_addr,res->ai_addrlen); 
        if(n==-1)
        {
            printf("Error: Unexpected connect\n");
            exit(1);
        }
        //-------------------------------- CONNECTED ------------------------------------
        ptr=strcpy(buffer,"Hello!\n"); 
        nbytes=7;
        nleft=nbytes; 

        while(nleft>0)
        {
            nwritten=write(fd,ptr,nleft);
            if(nwritten<=0)
            {
                printf("Error: Unexpected write\n");
                exit(1);
            } 
            nleft-=nwritten;
            ptr+=nwritten;
        }

        nleft=nbytes; 
        ptr=buffer; 
        
        while(nleft>0)
        {
            nread=read(fd,ptr,nleft);
            if(nread==-1)
            {
                printf("Error: Unexpected read\n");
                exit(1);
            }
            else if(nread==0) break;//closed by peer 
            nleft-=nread;
            ptr+=nread;
        }
        nread=nbytes-nleft;

        buffer[nread] = '\0'; 
        printf("echo: %s\n", buffer); 
        close(fd);
    }
    
    return;
}

int choose_extern()
{
    if (TCP_NodesCounter == 0)
    {
        //strcpy(node.extern_id = node.nodeid;
        strcpy(node.extern_IP, node.nodeIP);
        strcpy(node.extern_PORT, node.nodeTCP);

        //strcpy(node.backup_nid = node.nodeid;
        strcpy(node.backup_IP, node.nodeIP);
        strcpy(node.backup_PORT, node.nodeTCP);
        printf("estava sozinho\n");
        return(1);
    }

    else if (TCP_NodesCounter == 1)
    {
        //strcpy(node.extern_id = node.nodeid;
        strcpy(node.extern_IP, TCP_IParray[0]);
        strcpy(node.extern_PORT, TCP_PORTarray[0]);

        //strcpy(node.backup_nid = node.nodeid;
        strcpy(node.backup_IP, TCP_IParray[0]);
        strcpy(node.backup_PORT, TCP_PORTarray[0]);

        return(0);
    }
    
    //for (int i = 0; i < array_size; i++)
    //{
        /* code */
    //}
    return 0;
    
}

void prepare_tcpServer(struct addrinfo *ser_hints, struct addrinfo *ser_res, int *ser_listenfd){
    int errcode;
    
    if((*ser_listenfd=socket(AF_INET,SOCK_STREAM,0))==-1)
    {
        printf("Error: Unexpected TCP_serv socket\n");
        exit(1);
    }
    memset(ser_hints,0,sizeof (*ser_hints));
    ser_hints->ai_family=AF_INET;//IPv4
    ser_hints->ai_socktype=SOCK_STREAM;//TCP socket
    ser_hints->ai_flags=AI_PASSIVE; 
    if((errcode=getaddrinfo(NULL, node.nodeTCP,ser_hints,&ser_res))!=0)    //meter aqui o TCP do no escolhido 
    {
        printf("Error: Unexpected getaddrinfo serv\n");
        exit(1);
    }
    if(bind(*ser_listenfd, ser_res->ai_addr,ser_res->ai_addrlen)==-1)
    {
        printf("Error: Unexpected bind\n");
        exit(1);
    }
    if(listen(*ser_listenfd,5)==-1)
    {
        printf("Error: Unexpected listen\n");
        exit(1);
    }
    printf("Server side prepared\n");
}

void tcp_serv ()
{
    struct addrinfo hints,*res;
    ssize_t n, nw;
    int fd,newfd,errcode;
    struct sockaddr addr;
    socklen_t addrlen; 
    char *ptr,buffer[128];
    if((fd=socket(AF_INET,SOCK_STREAM,0))==-1)
    {
        printf("Error: Unexpected TCP_serv socket\n");
        exit(1);
    }
    memset(&hints,0,sizeof hints);
    hints.ai_family=AF_INET;//IPv4
    hints.ai_socktype=SOCK_STREAM;//TCP socket
    hints.ai_flags=AI_PASSIVE; 
    if((errcode=getaddrinfo(NULL, node.nodeTCP,&hints,&res))!=0)    //meter aqui o TCP do no escolhido 
    {
        printf("Error: Unexpected getaddrinfo serv\n");
        exit(1);
    }
    if(bind(fd,res->ai_addr,res->ai_addrlen)==-1)
    {
        printf("Error: Unexpected bind\n");
        exit(1);
    }
    if(listen(fd,5)==-1)
    {
        printf("Error: Unexpected listen\n");
        exit(1);
    }

    while(1)
    {
        addrlen=sizeof(addr); 
        if((newfd=accept(fd,&addr,&addrlen))==-1)
            {
                printf("Error: Unexpected accept\n");
                exit(1);
            } 
        
        while((n=read(newfd,buffer,128))!=0)
        {
            if(n==-1)
            {
                printf("Error: Unexpected read\n");
                exit(1);
            } 
            ptr=&buffer[0];
            while(n>0)
            {
                if((nw=write(newfd,ptr,n))<=0)
                {
                    printf("Error: Unexpected write\n");
                    exit(1);
                } 
                n-=nw; 
                ptr+=nw;
            }
            close(newfd);
        } 
    } 

}

void print_TCParray(){
    int i = 0;

    for(i = 0; i<TCP_NodesCounter; i++){
        printf("Array[%d]::  IP: %s   Port: %s\n\n", i, TCP_IParray[i], TCP_PORTarray[i]);
    }
}


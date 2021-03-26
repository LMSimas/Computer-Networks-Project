#include "headerFile.h"
#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 1024





int main (int argc, char* argv[]){

    enum {notreg} state;
    fd_set rfds;
    int maxfd, counter;

    /*VALIDATION STEP*/
    if(!validate_inputArgs(argc, argv)) // if NOT validated
        exit(1);//in order 2 exit the programm

    /*APPLICATION STARTS HERE*/
    printf("ndn> "); fflush(stdout); //prompt
    state = notreg;
        while(1)
        {
            FD_ZERO(&rfds);
            switch (state)
            {
            case notreg: FD_SET (0, &rfds); maxfd=0; break;
                /* code */
                break;
            
            default:
                break;
            }

            counter=select(maxfd+1,&rfds,(fd_set*)NULL,(fd_set*)NULL,(struct timeval *)NULL);
            if(counter<=0)/*error*/exit(1);
            
        }//while(1)


    if (fgets(buffer, BUFFER_SIZE, stdin)!=NULL) printf("Command: %s \n", buffer);

    return 0;
}


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
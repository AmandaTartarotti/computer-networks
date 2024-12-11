/**      (C)2000-2021 FEUP
 *       tidy up some includes and parameters
 * */

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>
#include "aux_functions.h"

#define SERVER_PORT 21
#define SERVER_ADDR "193.137.29.15"

#define MAX_LENGTH 1000

typedef enum{
    START,
    EXIT,
    WRITE_USER,
    READ_STATUS,
    WRITE_PASS,
    FINISH,
} state;


int main(int argc, char **argv) {

    char username[MAX_LENGTH];
    char password[MAX_LENGTH];
    char host[MAX_LENGTH];
    char urlpath[MAX_LENGTH];
    int sockfd;
    size_t bytes;

    if (argc != 2) {
        printf("Usage: %s %s\n", argv[0], "ftp://[<user>:<password>@]<host>/<url-path>");
        return -1;
    }

    if (get_url_info(argv[1], username, password, host, urlpath) != 0) {
        return -1;
    }

    if((sockfd = connectSocket(SERVER_ADDR, SERVER_PORT)) < 0){
        return -1;
    }

    char status[3];
    printf("$client: starts connection with the server\n");
    sleep(0.5);
    if(readServer(sockfd, status)<0){
        printf("failed read");
    };
    
    if(strcmp(status,"220")){
        bytes = write(sockfd, "USER anonymous\n", strlen("USER anonymous\n"));
        printf("$client: %s", "USER anonymous\n");
    }

    if(readServer(sockfd, status)<0){
        printf("failed read");
    };

    if(strcmp(status,"331")){
        bytes = write(sockfd, "PASS anonymous\n", strlen("PASS anonymous\n"));
        printf("$client: %s", "PASS anonymous\n");
    }

    if(readServer(sockfd, status)<0){
        printf("failed read");
    };

    if(strcmp(status,"230")){
        bytes = write(sockfd, "pasv\n", strlen("pasv\n"));
        printf("$client: %s", "pasv\n");
    }

    if(readServer(sockfd, status)<0){
        printf("failed read");
    };


    printf("\n >>> Fim da máquina de estados <<<");


    if (close(sockfd)<0) {
        perror("close()");
        exit(-1);
    }
    return 0;
}



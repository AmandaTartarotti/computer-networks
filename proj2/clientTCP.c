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

    if (argc != 2) {
        printf("Usage: %s %s\n", argv[0], "ftp://[<user>:<password>@]<host>/<url-path>");
        return -1;
    }

    if (get_url_info(argv[1], username, password, host, urlpath) != 0) {
        return -1;
    }

    int sockfd;
    struct sockaddr_in server_addr;

    size_t bytes;

    /*server address handling*/
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);    /*32 bit Internet address network byte ordered*/
    server_addr.sin_port = htons(SERVER_PORT);        /*server TCP port must be network byte ordered */

    /*open a TCP socket*/
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        exit(-1);
    }
    /*connect to the server*/
    if (connect(sockfd,
                (struct sockaddr *) &server_addr,
                sizeof(server_addr)) < 0) {
        perror("connect()");
        exit(-1);
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



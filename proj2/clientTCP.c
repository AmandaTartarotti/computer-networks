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

typedef enum
{
    START,
    EXIT,
    WRITE_USER,
    READ_STATUS,
    WRITE_PASS,
    FINISH,
} state;

int main(int argc, char **argv)
{

    char username[MAX_LENGTH];
    char password[MAX_LENGTH];
    char host[MAX_LENGTH];
    char urlpath[MAX_LENGTH];
    int sockfd;

    if (argc != 2)
    {
        printf("Usage: %s %s\n", argv[0], "ftp://[<user>:<password>@]<host>/<url-path>");
        return -1;
    }

    if (get_url_info(argv[1], username, password, host, urlpath) != 0)
        return -1;

    if (get_ip(host) != 0)
        return -1;

    if ((sockfd = connectSocket(host, SERVER_PORT)) < 0)
    {
        return -1;
    }

    char status[4];
    printf("$client: starts connection with the server\n");
    sleep(0.5);

    if (readServer(sockfd, status) < 0)
    {
        printf("failed read");
    };

    if (strcmp(status, "220") == 0)
    {
        sendCommandToServer(sockfd, "USER", username);
    }
    else
    {
        printf("Failed in the connection\n");
        return -1;
    }

    if (readServer(sockfd, status) < 0)
    {
        printf("failed read");
        return -1;
    };

    if (strcmp(status, "331") == 0)
    {
        sendCommandToServer(sockfd, "PASS", password);
    }
    else
    {
        printf("Failed in the user connect\n");
        return -1;
    }

    if (readServer(sockfd, status) < 0)
    {
        printf("failed read");
        return -1;
    };

    if (strcmp(status, "230") == 0)
    {
        sendCommandToServer(sockfd, "pasv", "");
    }
    else
    {
        printf("Failed in the password\n");
        return -1;
    }

    if (retrieveFile(sockfd, urlpath, host) != 0)
    {
        printf("failed retrieve\n");
        return -1;
    }

    if (readServer(sockfd, status) < 0)
    {
        printf("failed read");
        return -1;
    };


    if (close(sockfd) < 0)
    {
        perror("close()");
        exit(-1);
    }

    printf("\n >>>Fim<<<\n");
    
    return 0;
}

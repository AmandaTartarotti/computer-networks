
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include "aux_functions.h"


int get_url_info(char *argv, char *username, char *password, char *host, char *urlpath)
{
  if (strstr(argv, "ftp://") == argv) {
    char *after_prefix = argv + 6;

    if (sscanf(after_prefix, "%49[^:]:%49[^@]@%99[^/]/%199s", username, password, host, urlpath) == 4) {
      printf("User: %s\n", username);
      printf("Password: %s\n", password);
      printf("Host: %s\n", host);
      printf("URL Path: %s\n", urlpath);
    }
    else if (sscanf(after_prefix, "%99[^/]/%199s", host, urlpath) == 2) {
            strcpy(username, "anonymous");
            strcpy(password, "anonymous");
            printf("User: %s\n", username);
            printf("Password: %s\n", password);
            printf("Host: %s\n", host);
            printf("URL Path: %s\n", urlpath);
    }
    else {
      printf("Invalid FTP URL format.\n");
      return -1;
    }

  } 

  else {
    printf("Not an FTP URL.\n");
    return -1;
  }
  return 0;

}

int connectSocket(char *address, int port) {
  int sockfd;
    struct sockaddr_in server_addr;

    /*server address handling*/
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(address);    /*32 bit Internet address network byte ordered*/
    server_addr.sin_port = htons(port);        /*server TCP port must be network byte ordered */

    /*open a TCP socket*/
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        return -1;
    }
    /*connect to the server*/
    if (connect(sockfd,
                (struct sockaddr *) &server_addr,
                sizeof(server_addr)) < 0) {
        perror("connect()");
        return -1;
    }

    return sockfd;
}

int readServer(int sockfd, char *status)
{
  char buf[1000];
  size_t bytes;
  memset (buf,' ',1000);
  bytes = read(sockfd, buf, strlen(buf) - 1);

  if (bytes > 1)
  {
    buf[bytes] ='\0';
    memcpy(status, buf, 3);
    printf("$Server: %s",buf);
  }
  else
    return -1;

  return 0;
}
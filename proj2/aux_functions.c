
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include "aux_functions.h"

int get_url_info(char *argv, char *username, char *password, char *host, char *urlpath)
{
  if (strstr(argv, "ftp://") == argv)
  {
    char *after_prefix = argv + 6;

    if (sscanf(after_prefix, "%49[^:]:%49[^@]@%99[^/]/%199s", username, password, host, urlpath) == 4)
    {
      printf("User: %s\n", username);
      printf("Password: %s\n", password);
      printf("Host: %s\n", host);
      printf("URL Path: %s\n", urlpath);
    }
    else if (sscanf(after_prefix, "%99[^/]/%199s", host, urlpath) == 2)
    {
      strcpy(username, "anonymous");
      strcpy(password, "anonymous");
      printf("User: %s\n", username);
      printf("Password: %s\n", password);
      printf("Host: %s\n", host);
      printf("URL Path: %s\n", urlpath);
    }
    else
    {
      printf("Invalid FTP URL format.\n");
      return -1;
    }
  }

  else
  {
    printf("Not an FTP URL.\n");
    return -1;
  }
  return 0;
}

int get_ip(char *host)
{
  struct hostent *h;
  if ((h = gethostbyname(host)) == NULL)
  {
    herror("gethostbyname()");
    return -1;
  }

  strcpy(host, inet_ntoa(*((struct in_addr *)h->h_addr)));

  return 0;
}

int connectSocket(char *address, int port)
{
  int sockfd;
  struct sockaddr_in server_addr;

  /*server address handling*/
  bzero((char *)&server_addr, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(address); /*32 bit Internet address network byte ordered*/
  server_addr.sin_port = htons(port);               /*server TCP port must be network byte ordered */

  /*open a TCP socket*/
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("socket()");
    return -1;
  }
  /*connect to the server*/
  if (connect(sockfd,
              (struct sockaddr *)&server_addr,
              sizeof(server_addr)) < 0)
  {
    perror("connect()");
    return -1;
  }

  return sockfd;
}

int readServer(int sockfd, char *status)
{
  char buf[1000];
  size_t bytes;
  memset(buf, ' ', 1000);
  bytes = read(sockfd, buf, strlen(buf) - 1);

  if (bytes > 1)
  {
    buf[bytes] = '\0';
    memcpy(status, buf, 3);
    printf("$Server: %s", buf);
  }
  else
    return -1;

  return 0;
}

int sendCommandToServer(int sockfd, char *cmd, char *body)
{
  char aux[1000] = {0};

  strcpy(aux, cmd);
  strcat(aux, " ");
  strcat(aux, body);
  strcat(aux, "\n");

  if (write(sockfd, aux, strlen(aux)) < 0)
    return -1;

  printf("$client: %s", aux);

  return 0;
}

int retrieveFile(int sockfd, char *urlpath, char *address)
{
  char buf[1024];
  memset(buf, ' ', 1024);
  char status[4];
  size_t bytes;
  if((bytes = read(sockfd, buf, strlen(buf) - 1))< 1){
    return -1;
  }
  buf[bytes] = '\0';
  printf("$server:%s", buf);

  memcpy(status, buf, 3);

  if(strcmp(status, "227")){
    sendCommandToServer(sockfd, "RETR", urlpath);
  }

  //Gets the right port to connect the other socket
  int p1, p2;
  sscanf(buf, "227 Entering Passive Mode (193,137,29,15,%d,%d).", &p1, &p2);
  int port = p1 * 256 + p2;

  int sockfile = connectSocket(address, port);

  //gets the filename to create the file
  char filename[256];
  sscanf(urlpath, "%*[^/]/%255[^\n]", filename);
  printf("Downloading file %s\n", filename);

  FILE *fp = fopen(filename, "w");
  if (fp == NULL){
      printf("Error opening or creating file\n");
      return -1;
  }

  char filebuf[1024];
  int n_bytes;

  while((n_bytes = read(sockfile, filebuf, sizeof(filebuf)))){
        if(n_bytes < 0){
            printf("Error reading from data socket\n");
            return -1;
        }
        if((n_bytes = fwrite(filebuf, n_bytes, 1, fp)) < 0){
            printf("Error writing data to file\n");
            return -1;
        }
    }
    printf("Finished dowloading File\n");
    if(fclose(fp) < 0){
        printf("Error closing file\n");
        return -1;
    }

    if (close(sockfile)<0) {
        perror("close()");
        exit(-1);
    }


  return 0;
}
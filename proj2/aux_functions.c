
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include "aux_functions.h"

int recived_status(unsigned char *buf, unsigned char *status)
{

  for (int i = 0; i < 3; i++)
  {
    if (status[i] != buf[i])
      return -1;
  }

  return 0;
}

int get_url_info()
{
  return -1;
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
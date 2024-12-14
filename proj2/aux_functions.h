#ifndef AUX_FUNCTIONS_H
#define AUX_FUNCTIONS_H

/**
 * Receive from input "ftp://[<user>:<password>@]<host>/<url-path>"
 * 
 * @return 0 uppon sucess and -1 if fails
 */
int get_url_info(char *argv, char *username, char *password, char *host, char *urlpath);

int get_ip(char *host);

int connectSocket(char *address, int port);

/**
 * Checks if sucess of a performed operation
 * @param buf pointer to a recived operation menssage
 * @param status pointer to an operation expected status
 * @return 0 uppon sucess and -1 if fails
 */
int recived_status(unsigned char* buf, unsigned char* status);


int readServer(int sockfd, char *status);

int sendCommandToServer(int sockfd, char *cmd, char *body);

int retrieveFile(int sockfd, char *urlpath, char *address);

#endif
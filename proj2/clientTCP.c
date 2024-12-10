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

/**
 * Checks if sucess of a performed operation
 * @param buf pointer to a recived operation menssage
 * @param status pointer to an operation expected status
 * @return 0 uppon sucess and -1 if fails
 */
int recived_status(unsigned char* buf, unsigned char* status){

    for(int i = 0; i < 3; i++){
        if (status[i] !=  buf[i])
            return -1;
    }

    return 0;
}

/**
 * Receive from input "ftp://[<user>:<password>@]<host>/<url-path>"
 * 
 * @return 0 uppon sucess and -1 if fails
 */
int get_url_info(){
    return -1;
}

int main(int argc, char **argv) {

    if (argc > 1)
        printf("**** No arguments needed. They will be ignored. Carrying ON.\n");

    int sockfd;
    struct sockaddr_in server_addr;

    char buf[MAX_LENGTH];
    char stat_buf[100];

    size_t bytes;

    int get_status;
    size_t auth_bytes;
    size_t stat_bytes;

    char status_200[] = "220";
    char status_331[] = "331";
    char status_230[] = "230";

    char username_buf[] = "USER anonymous";
    char password_buf[] = "PASS anonymous";

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

    state cur_state = START;
 
    //State machine implementation
    while (cur_state != FINISH){
        switch(cur_state){

            case START:
                memset (buf,' ',MAX_LENGTH);
                bytes = read(sockfd, buf, strlen(buf) - 1);

                if (bytes < 1) perror("read()");

                //To debug
                printf("Number os bytes read: %ld\n", bytes);
                printf("Received message: %s\n", buf);

                get_status = recived_status(buf, status_200);
                if(!get_status) cur_state = WRITE_USER;
                else cur_state = EXIT;

                break; 

            case WRITE_USER:
                printf("Process started with success!\n");
                printf("------------------------------\n");

                auth_bytes = write(sockfd, username_buf, strlen(username_buf));

                if (auth_bytes < 1) printf("Could not write username");

                //To debug
                if (auth_bytes > 0) {
                    printf("Number os bytes write: %ld\n", auth_bytes);
                    printf("Writed message: %s\n", username_buf);
                }

                cur_state = READ_STATUS;
                break;

            case READ_STATUS:

                memset (stat_buf,' ',100);
                printf("Line 126\n");
                stat_bytes = read(sockfd, stat_buf, strlen(stat_buf) - 1);
                printf("Line 128\n");
                if (stat_bytes < 1) perror("read()");

                //To debug
                printf("Number os bytes read: %ld\n", stat_bytes);
                printf("Read message: %s\n", stat_buf);

                get_status = recived_status(buf, status_331);
                if(!get_status) cur_state = WRITE_PASS;
                else cur_state = EXIT;

                break;
            
            case WRITE_PASS:
                printf("Login on server..");

                auth_bytes = write(sockfd, password_buf, strlen(username_buf));
                
                 //To debug
                printf("Number os bytes read: %ld\n", auth_bytes);
                printf("Read message: %s\n", stat_buf);

                get_status = recived_status(buf, status_331);
                if(!get_status) printf("Login on server..");

                cur_state = FINISH;

                break;

            case EXIT:
                printf("\nOh no! Something went wrong :c\nAborting operation\n");
                exit(-1);
                break;

            default:
                break;
        }
    }

    printf("\n >>> Fim da máquina de estados <<<");

    /*send a string to the server*/

    ////////////////// To be deleted after state machine implementation /////////////

    bytes = read(sockfd, buf, strlen(buf)); 
    if (bytes > 0){
        printf("\nBytes lidos %ld\n", bytes);

        for(int i = 0; i < bytes; i++){
            printf("%c", buf[i]);
        }

        auth_bytes = write(sockfd, username_buf, strlen(username_buf));
        if (auth_bytes > 0){
            printf("\nBytes escritos - username %ld\n", auth_bytes);

            //Le a resposta se escreveu o username bem
            auth_bytes = read(sockfd, buf, strlen(buf));
            if (bytes > 0){

                for(int i = 0; i < 3; i++)
                    printf("%c", buf[i]);

                auth_bytes = write(sockfd, password_buf, strlen(password_buf));
                if (auth_bytes > 0)
                    printf("\nBytes escritos - password %ld\n", auth_bytes);
            }
        }
        else {
            printf("\nError during username write! Aborting operation\n");
            exit(-1);
        }
    }

    ////////////////////////////////////////////////////////////////////////////

    else {
        perror("read()");
        exit(-1);
    }

    if (close(sockfd)<0) {
        perror("close()");
        exit(-1);
    }
    return 0;
}



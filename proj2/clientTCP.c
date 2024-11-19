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
 * @return 0 uppon sucess and -1 if fails
 */
int recived_status(unsigned char* buf){
    char sucess_status[] = "220";

    for(int i = 0; i < 3; i++){
        if (sucess_status[i] !=  buf[i])
            return -1;
    }

    return 0;
}

int main(int argc, char **argv) {

    if (argc > 1)
        printf("**** No arguments needed. They will be ignored. Carrying ON.\n");
    int sockfd;
    struct sockaddr_in server_addr;
    char buf[] = "Mensagem de teste na travessia da pilha TCP/IP\n";
    size_t bytes;

    int get_status;
    char username_buf[] = "USER anonymous";
    size_t auth_bytes;

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
                bytes = read(sockfd, buf, strlen(buf));
                //read until 0 bytes -- to be implemented

                get_status = recived_status(buf);
                if(!get_status) cur_state = WRITE_USER;
                else cur_state = EXIT;

                break; 

            case WRITE_USER:
                printf("Process started with success!\n");
                printf("------------------------------\n");
                auth_bytes = write(sockfd, username_buf, strlen(username_buf));
                if (auth_bytes > 0) printf("\nBytes escritos - username %ld\n", auth_bytes);
                cur_state = READ_STATUS;
                break;

            case READ_STATUS:
                //in implementation -> codigo para testes:
                char stat_buf[100] = "asdfghjklasdfghjk";
                auth_bytes = read(sockfd, stat_buf, strlen(stat_buf));

                if (auth_bytes < 1) printf("Could not read status");

                for(int i = 0; i < auth_bytes; i++)
                    printf("%c", stat_buf[i]);

                cur_state = FINISH;

                // get_status = recived_status(stat_buf);
                // if(!get_status) cur_state = WRITE_PASS;
                // else cur_state = EXIT;

                break;
            
            case WRITE_PASS:
                auth_bytes = write(sockfd, password_buf, strlen(username_buf));
                if (auth_bytes > 0) printf("\nBytes escritos - passowrd %ld\n", auth_bytes);
                //cur_state = ;
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



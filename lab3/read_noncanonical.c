// Read from serial port in non-canonical mode
//
// Modified by: Eduardo Nuno Almeida [enalmeida@fe.up.pt]

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1

#define FLAG 0x7E
#define A_R 0x01
#define C_R 0x07

#define BUF_SIZE 256  

#define A_S 0x03
#define A_C 0x03

int data_size = 3;
unsigned char data_field[3] = {0x20, 0x21, 0x22}; 

// Na verdade ele deveria calculcar o bcc com base nos valores que ele recebe 
unsigned char bcc_definer() {
    unsigned char bcc2 = 0x00;
    for (int i = 0; i < data_size; i++){
        bcc2 = bcc2 ^ data_field[i];
    }

    return bcc2;
}

// Definir maquina de estados
typedef enum {
  START,
  FLAG_RCV,
  A_RCV,
  C_RCV,
  BCC,
  BCC2,
  STOP_RCV
} state_set_msg;

state_set_msg cur_state = START;

volatile int STOP = FALSE;

int main(int argc, char *argv[])
{
    // Program usage: Uses either COM1 or COM2
    const char *serialPortName = argv[1];

    if (argc < 2)
    {
        printf("Incorrect program usage\n"
               "Usage: %s <SerialPort>\n"
               "Example: %s /dev/ttyS1\n",
               argv[0],
               argv[0]);
        exit(1);
    }

    // Open serial port device for reading and writing and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    int fd = open(serialPortName, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(serialPortName);
        exit(-1);
    }

    struct termios oldtio;
    struct termios newtio;

    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0; // Inter-character timer unused
    newtio.c_cc[VMIN] = 10;  // Blocking read until 5 chars received

    // VTIME e VMIN should be changed in order to protect with a
    // timeout the reception of the following character(s)

    // Now clean the line and activate the settings for the port
    // tcflush() discards data written to the object referred to
    // by fd but not transmitted, or data received but not read,
    // depending on the value of queue_selector:
    //   TCIFLUSH - flushes data received but not read.
    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");

    // Loop for input
    unsigned char buf[BUF_SIZE + 1] = {0}; // +1: Save space for the final '\0' char

    while (STOP == FALSE){
        // Returns after 5 chars have been input
        int bytes = read(fd, buf, BUF_SIZE);
        buf[bytes] = '\0'; // Set end of string to '\0', so we can printf
        int index;

        while (cur_state != STOP_RCV){
        
            //Confere sucesso da FLAG e da mensagem com State Machine
            switch(cur_state){
                case START:
                    printf("Current state = START.\n");
                    index = 0;
                    //se a flag for recebida corretamente, avança para FLAG_RCV e proximo byte
                    if (buf[index] == FLAG){
                        cur_state = FLAG_RCV;
                        index++;
                    }
                    break;

                case FLAG_RCV:
                    printf("Current state = FLAG_RCV.\n");
                    //Se A_S for recebido com sucesso avança para A_RCV e proximo byte
                    if (buf[index] == A_S){
                        cur_state = A_RCV;
                        index++;
                    }
                    // se a flag for recebida de novo, avança para proximo byte
                    else if (buf[index] == FLAG)
                        index++;
                    // se recebe um valor desconhecido, volta ao START
                    else 
                        cur_state = START;
                    break;

                case A_RCV:
                    printf("Current state = A_RCV.\n");
                    //Se flag for recebida, volta para FLAG e avança para próximo byte
                    if (buf[index] == FLAG){
                        cur_state = FLAG_RCV;
                        index++;
                    }
                    //Se A_C for recebido com sucesso avança para C_RCV e proximo byte
                    else if (buf[index] == A_C){
                        cur_state = C_RCV;
                        index++;
                    }
                    else 
                        cur_state = START;
                    break;

                case C_RCV:
                    printf("Current state = C_RCV.\n");
                    //Se flag for recebida, volta para FLAG e avança para próximo byte
                    if (buf[index] == FLAG){
                        cur_state = FLAG_RCV;
                        index++;
                    }
                    //Se A_C^A_S for recebido com sucesso avança para BCC e proximo byte
                    else if (buf[index] == A_C^A_S){
                        cur_state = BCC;
                        index++;
                    }
                    else 
                        cur_state = START;
                    break;

                case BCC:
                    printf("Current state = BCC.\n");
                    unsigned char bcc2 = bcc_definer();
                    
                    //Espera pelo data_size de 3 e confere o próximo resultado, vulgo o BCC2
                    index += data_size;

                    if (buf[index] == FLAG){
                        cur_state = FLAG_RCV;
                        index++;
                    }
                    if (buf[index] == bcc2){
                        cur_state = BCC2;
                        index++;
                    }
                    else 
                        cur_state = STOP_RCV;
                        // cur_state = START;
                    break;
                
                case BCC2:
                    printf("Current state = BCC2.\n");
                    //Se flag for recebida com sucesso avança para STOP e para próximo byte
                    if (buf[index] == FLAG){
                        printf("Mensagem recebida com sucesso.\n");
                        cur_state = STOP_RCV;
                        index++;
                    }
                    else 
                        cur_state = START;
                    break;

                case STOP_RCV:
                    break;
                default:
                    printf("Mensagem não recebida corretamente\n");
                    break;
            }
    }

        // Create string to send
        unsigned char buf_UA[BUF_SIZE] = {0};

        //Mensagem a enviar
        buf_UA[0] = FLAG;
        buf_UA[1] = A_R;
        buf_UA[2] = C_R;
        buf_UA[3] = 0xFF;
        // buf_UA[3] = A_R ^ C_R;
        buf_UA[4] = FLAG; 

        int bytes_UA = write(fd, buf_UA, BUF_SIZE);
        printf("%d bytes written:\n", bytes);

        for (int i = 0; i < 5; i++){
            printf("0x%02X\n", buf_UA[i]);
        }

        printf("%d bytes read\n", bytes);
        printf(":%s:%d\n", buf, bytes);
        if (buf[bytes] == '\0')
            STOP = TRUE;
    }

    // The while() cycle should be changed in order to respect the specifications
    // of the protocol indicated in the Lab guide
    sleep(1);
    // Restore the old port settings
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);

    return 0;
}
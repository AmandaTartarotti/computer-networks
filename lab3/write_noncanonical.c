// Write to serial port in non-canonical mode
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

//from the alarm
#include <signal.h>

// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1

#define BUF_SIZE 256

#define FLAG 0x7e
#define A_S 0x03
#define A_C 0x03


#define A_R 0x01
#define C_R 0x07

typedef enum {
  START,
  FLAG_RCV,
  A_RCV,
  C_RCV,
  BCC,
  DATA,
  STOP_RCV
} state_set_msg;

volatile int STOP = FALSE;

int alarmEnabled = FALSE;
int alarmCount = 0;

int fd = -1;

int data_size = 3;
unsigned char data_field[3] = {0x20, 0x21, 0x22}; 

int writeData(){
    // Create string to send
    unsigned char buf[BUF_SIZE] = {0};

    //Mensagem a enviar
    buf[0] = FLAG;
    buf[1] = A_S;
    buf[2] = A_S;
    buf[3] = A_S ^ A_S;

    //Envia o data field
    int index = 4; 
    unsigned char bbc2 = 0x00;
    for (int i = 0; i < data_size; i++){
        buf[index + i] = data_field[i];
        bbc2 = bbc2 ^ data_field[i];
    }
    index += data_size;

    buf[index] = bbc2;

    //Volta o resto do código como antes 
    buf[index + 1] = FLAG; 

    //writes the msg
    int bytes = write(fd, buf, index + 2);
    printf("%d bytes written\n", bytes);

    return 0;
}

int writeSet(){

    // Create string to send
    unsigned char buf[5];

    //Mensagem a enviar
    buf[0] = FLAG;
    buf[1] = A_S;
    buf[2] = A_S;
    buf[3] = A_S ^ A_S;
    buf[4] = FLAG; 

    //writes the msg
    int bytes = write(fd, buf, 5);

    printf("%d bytes written\n", bytes);

    return 0;
}

int readMsg(){
 
    // Recebe e mostra o que o reader retornou
    unsigned char buf_UA[BUF_SIZE + 1] = {0};

    int bytes_UA = read(fd, buf_UA, 5);
    buf_UA[bytes_UA] = '\0';

    //Confere sucesso da FLAG
    if (buf_UA[0] == FLAG && 
        buf_UA[4] == FLAG && 
        buf_UA[1] == A_R && 
        buf_UA[2] == C_R && 
        buf_UA[3]==(A_R^C_R)){
        
        printf("Mensagem UA recebida com sucesso.\n");
        if (buf_UA[bytes_UA] == '\0') {
            return 0;
        }
    }
    else{
        printf("Mensagem UA não recebida corretamente:\n");
        for (int i = 0; i < 5; i++){
            printf("0x%02X\n", buf_UA[i]);
        }
        return 1;
    }
}

// Alarm function handler
void alarmHandler(int signal)
{   
    alarmEnabled = FALSE;
    alarmCount++;

    //printf("Alarm #%d\n", alarmCount);

}

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

    // Open serial port device for reading and writing, and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    fd = open(serialPortName, O_RDWR | O_NOCTTY);

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
    newtio.c_cc[VTIME] = 0.5; // Inter-character timer unused
    newtio.c_cc[VMIN] = 0;  // Blocking read until 5 chars received

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
    
    (void)signal(SIGALRM, alarmHandler);

    state_set_msg current_state = START;
    unsigned char byte;

    while (alarmCount < 4 && current_state != STOP_RCV) {
        printf("Alarm count: %d\n", alarmCount);
        writeSet();
        alarm(5);
        alarmEnabled = TRUE;
        while(current_state != STOP_RCV && alarmEnabled==TRUE) {
            if(read(fd, &byte, 1)){
                printf("buffer -- 0x%02X\n", byte);
                switch (current_state)
                {
                case START:
                    printf("Current state = START.\n");
                    if (byte == FLAG) current_state = FLAG_RCV;
                    break;

                case FLAG_RCV:
                    printf("Current state = FLAG_RCV.\n");
                    if (byte == A_R) current_state = A_RCV;
                    else if (byte == FLAG) break;
                    else current_state = START;
                    break;

                case A_RCV:
                    printf("Current state = A_RCV.\n");
                    if (byte == FLAG) current_state = FLAG_RCV;
                    else if (byte == C_R) current_state = C_RCV;
                    else current_state = START;
                    break;

                case C_RCV:
                    printf("Current state = C_RCV.\n");
                    if (byte == FLAG) current_state = FLAG_RCV;
                    else if (byte == A_R^C_R) current_state = BCC;
                    else current_state = START;
                    break;

                case BCC:
                    printf("Current state = BCC.\n");
                    if (byte == FLAG){
                        current_state = STOP_RCV;
                        printf("UA RECEBIDO!!");
                    }
                    else 
                        current_state = START;
                    break;
                default:
                    break;
                }
            }
        }
    }

    // if(writeData())
    //     printf("Write data falhou\n");

    printf("Ending program\n");

    // Restore the old port settings
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);

    return 0;
}

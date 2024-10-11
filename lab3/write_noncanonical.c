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
#include "alarm.c"

extern int alarmCount;
extern int alarmEnabled;

// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1

#define BUF_SIZE 256

#define FLAG 0x7e
#define A_S 0x03

#define A_R 0x01
#define C_R 0x07

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

    // Open serial port device for reading and writing, and not as controlling tty
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
    newtio.c_cc[VMIN] = 5;  // Blocking read until 5 chars received

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

    // Create string to send
    unsigned char buf[BUF_SIZE] = {0};

    //Mensagem a enviar
    buf[0] = FLAG;
    buf[1] = A_S;
    buf[2] = A_S;
    buf[3] = A_S ^ A_S;
    buf[4] = FLAG; 
    buf[5] = '\n';

    (void)signal(SIGALRM, alarmHandler);

    while (alarmCount < 4 && alarmEnabled == FALSE){

            // Set alarm to be triggered in 3s
            alarm(3);

            //writes the msg
            int bytes = write(fd, buf, 5);
            printf("%d bytes written\n", bytes);

            // Wait until all bytes have been written to the serial port
            sleep(1);

            // Recebe e mostra o que o reader retornou
            unsigned char buf_UA[BUF_SIZE + 1] = {0};

            int bytes_UA = read(fd, buf_UA, 5);
            buf_UA[bytes_UA] = '\0';

            //Confere sucesso da FLAG
            if (buf_UA[0] == FLAG && 
                buf_UA[4] == FLAG && 
                buf_UA[1] == A_R && 
                buf_UA[2] == C_R && 
                buf_UA[3]==A_R^C_R)
                
                printf("Mensagem UA recebida com sucesso.\n");
            else
                printf("Mensagem UA não recebida corretamente\n");

            printf("%d bytes read\n", bytes_UA);
            printf(":%s:%d\n", buf, bytes);
            if (buf_UA[bytes_UA] == '\0') {
                alarmEnabled = TRUE;
                alarm(0);
                printf("Remaining alarms were disable\n");
            }

    }

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

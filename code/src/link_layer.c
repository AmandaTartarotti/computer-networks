// Link layer protocol implementation

#include "link_layer.h"
#include "serial_port.h"
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>


// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FLAG 0x7e

#define A_T 0x03 //commands sent by the Transmitter or replies sent by the Receiver
#define A_R 0x01 //commands sent by the Receiver or replies sent by the Transmitter

#define C_SET 0x03
#define C_UA 0x07
#define C_RR0 0xAA
#define C_RR1 0xAB
#define C_REJ0 0x54
#define C_REJ1 0x55
#define C_DISC 0x0B

#define ESC 0x70

int alarmEnabled = FALSE;
int alarmCount = 0;
unsigned char frameTx = 0;
unsigned char byte;
int totalRetransmitions, timeout;

void alarmHandler(int signal)
{   
    alarmEnabled = FALSE;
    alarmCount++;
}


int sendControlFrame (unsigned char A, unsigned char C){
    unsigned char frame[5] = {FLAG, A, C, A^C, FLAG};
    return writeBytesSerialPort(frame, 5);
}


typedef enum {
  START,
  FLAG_RCV,
  A_RCV,
  C_RCV,
  BCC1_OK,
  //DATA,
  //DATA_FOUND_ESC,
  //BCC2_OK,
  STOP_RCV
} state;

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{   
    if (openSerialPort(connectionParameters.serialPort,
                       connectionParameters.baudRate) < 0)
    {
        return -1;
    }
    state current_state = START;
    totalRetransmitions = connectionParameters.nRetransmissions;
    timeout = connectionParameters.timeout;
    switch (connectionParameters.role)
    {
    case LlTx: //Transmitter

        (void)signal(SIGALRM, alarmHandler);
        alarmCount = 0;

        while (alarmCount < totalRetransmitions && current_state != STOP_RCV) {
            sendControlFrame(A_T, C_SET);
            printf("Transmission: %d\n", alarmCount+1);

            alarm(timeout);
            alarmEnabled = TRUE;

            while(current_state != STOP_RCV && alarmEnabled==TRUE) {
                if(readByteSerialPort(&byte)) {
                    printf("buffer -- 0x%02X\n", byte);
                    switch (current_state)
                    {
                    case START:
                        if (byte == FLAG) current_state = FLAG_RCV;
                        break;

                    case FLAG_RCV:
                        if (byte == A_R) current_state = A_RCV;
                        else if (byte == FLAG) break;
                        else current_state = START;
                        break;

                    case A_RCV:
                        if (byte == FLAG) current_state = FLAG_RCV;
                        else if (byte == C_UA) current_state = C_RCV;
                        else current_state = START;
                        break;

                    case C_RCV:
                        if (byte == FLAG) current_state = FLAG_RCV;
                        else if (byte == (A_R^C_UA)) current_state = BCC1_OK;
                        else current_state = START;
                        break;

                    case BCC1_OK:
                        if (byte == FLAG){
                            current_state = STOP_RCV;
                            printf("UA RECEBIDO!!\n");
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
        break;
    case LlRx:
        while (current_state != STOP_RCV) {

            if(readByteSerialPort(&byte)) {
                printf("buffer -- 0x%02X\n", byte);
                switch (current_state)
                {
                case START:
                    if (byte == FLAG) current_state = FLAG_RCV;
                    break;

                case FLAG_RCV:
                    if (byte == A_T) current_state = A_RCV;
                    else if (byte == FLAG) break;
                    else current_state = START;
                    break;

                case A_RCV:
                    if (byte == FLAG) current_state = FLAG_RCV;
                    else if (byte == C_SET) current_state = C_RCV;
                    else current_state = START;
                    break;

                case C_RCV:
                    if (byte == FLAG) current_state = FLAG_RCV;
                    else if (byte == (A_T ^ C_SET)) current_state = BCC1_OK;
                    else current_state = START;
                    break;

                case BCC1_OK:
                    if (byte == FLAG)
                    {
                        current_state = STOP_RCV;
                        sendControlFrame(A_R, C_UA);
                        printf("Mensagem recebida com sucesso :)\n");
                    }
                    else current_state = START;
                    break;
                case STOP_RCV:
                    break;
                }
            }
        }
        break;
    default:
        break;
    }

    return 1;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    int framesize = bufSize+6;
    state current_state = START;
    alarmCount = 0;
    unsigned char cField = 0;
    unsigned char *frame = (unsigned char *) malloc(framesize);
    frame[0] = FLAG;
    frame[1] = A_T;
    frame[2] = frameTx ? 0x80 : 0x00;
    frame[3] = frame[1] ^ frame[2];

    unsigned char BCC2 = buf[0];
    for (int i = 0; i < bufSize; i++) BCC2 ^= buf[i];

    memcpy(frame+4, buf, bufSize);

    int j = 4;

    for (int i = 0; i < bufSize; i++) {
        if(buf[i] == FLAG || buf[i] == ESC) {
            framesize++;
            frame = realloc(frame, framesize);
            frame[j] = ESC;
            j++;
        }
        frame[j] = buf[i];
        j++;
    }

    frame[j] = BCC2;
    j++;
    frame[j] = FLAG;
    j++;

    while (alarmCount < totalRetransmitions && current_state != STOP_RCV) {
            writeBytesSerialPort(frame, j);
            printf("Transmission: %d\n", alarmCount+1);

            alarm(timeout);
            alarmEnabled = TRUE;

            while(current_state != STOP_RCV && alarmEnabled==TRUE) {
                if(readByteSerialPort(&byte)) {
                    printf("buffer -- 0x%02X\n", byte);
                    switch (current_state)
                    {
                    case START:
                        if (byte == FLAG) current_state = FLAG_RCV;
                        break;

                    case FLAG_RCV:
                        if (byte == A_R) current_state = A_RCV;
                        else if (byte == FLAG) break;
                        else current_state = START;
                        break;

                    case A_RCV:
                        if (byte == FLAG) current_state = FLAG_RCV;
                        else if (byte == C_RR0 || byte == C_RR1 || byte==C_REJ0 || byte==C_REJ1){
                            current_state = C_RCV;
                            cField = byte;
                        }
                        else current_state = START;
                        break;

                    case C_RCV:
                        if (byte == FLAG) current_state = FLAG_RCV;
                        else if (byte == (A_R^C_UA)) current_state = BCC1_OK;
                        else current_state = START;
                        break;

                    case BCC1_OK:
                        if (byte == FLAG){
                            current_state = STOP_RCV;
                        }
                        else 
                            current_state = START;
                        break;
                    default:
                        break;
                    }
                }
            }

            // if(!cField){
            //     continue;
            // }

            // elseif(cField == C_REJ0 || cField == C_REJ1){ A execução ja para no momento em que o state é STOP_RCV
            //     rejected = 1;
            // }

            if(cField == C_RR0 || cField == C_RR1){
                free(frame);
                frameTx = (frameTx+1) % 2;
                return framesize;
            }
        }
    free(frame);
    llclose(1);
    return -1;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    // TODO

    int clstat = closeSerialPort();
    return clstat;
}



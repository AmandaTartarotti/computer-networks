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

// Definition of usefull variables 
int alarmEnabled = FALSE;
int alarmCount = 0;
unsigned char frameX = 0;
unsigned char byte;
int totalRetransmitions, timeout;

struct Statistics {
    int NumberFramesSent;
    int numberRetransmissions;
    int numberTimeouts;
} statistics = {0,0,0};


////////////////////////////////////////////////
// Auxiliar functions
////////////////////////////////////////////////
void alarmHandler(int signal)
{   
    alarmEnabled = FALSE;
    alarmCount++;
    statistics.numberRetransmissions++;
}

/**
 * Sends the ACK frame
 * @param A adress field
 * @param C control field
 * @return returns -1 on error, otherwise the number of bytes written.
 */
int sendControlFrame (unsigned char A, unsigned char C){
    unsigned char frame[5] = {FLAG, A, C, A^C, FLAG};
    return writeBytesSerialPort(frame, 5);
}


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

    open_state current_state = START;
    totalRetransmitions = connectionParameters.nRetransmissions;
    timeout = connectionParameters.timeout;
    
    switch (connectionParameters.role)
    {
    case LlTx: //Transmitter

        (void)signal(SIGALRM, alarmHandler);
        alarmCount = 0;

        while (alarmCount < totalRetransmitions && current_state != STOP_RCV_open) {
            sendControlFrame(A_T, C_SET);
            printf("Transmission: %d\n", alarmCount+1);
            statistics.NumberFramesSent++;
            alarm(timeout);
            alarmEnabled = TRUE;

            while(current_state != STOP_RCV_open && alarmEnabled==TRUE) {
                if(readByteSerialPort(&byte)) {
                    switch (current_state)
                    {
                    case START_open:
                        if (byte == FLAG) current_state = FLAG_RCV_open;
                        break;

                    case FLAG_RCV_open:
                        if (byte == A_R) current_state = A_RCV_open;
                        else if (byte == FLAG) break;
                        else current_state = START_open;
                        break;

                    case A_RCV_open:
                        if (byte == FLAG) current_state = FLAG_RCV_open;
                        else if (byte == C_UA) current_state = C_RCV_open;
                        else current_state = START_open;
                        break;

                    case C_RCV_open:
                        if (byte == FLAG) current_state = FLAG_RCV_open;
                        else if (byte == (A_R^C_UA)) current_state = BCC1_OK_open;
                        else current_state = START_open;
                        break;

                    case BCC1_OK_open:
                        if (byte == FLAG){
                            current_state = STOP_RCV_open;
                            printf("UA recived\n");
                        }
                        else 
                            current_state = START_open;
                        break;
                    default:
                        break;
                    }
                }
            }
        }
        break;
    case LlRx:
        while (current_state != STOP_RCV_open) {

            if(readByteSerialPort(&byte)) {
                //printf("buffer -- 0x%02X\n", byte);
                switch (current_state)
                {
                case START_open:
                    if (byte == FLAG) current_state = FLAG_RCV_open;
                    break;

                case FLAG_RCV_open:
                    if (byte == A_T) current_state = A_RCV_open;
                    else if (byte == FLAG) break;
                    else current_state = START_open;
                    break;

                case A_RCV_open:
                    if (byte == FLAG) current_state = FLAG_RCV_open;
                    else if ((byte == 0x80) || (byte == 0x00)){
                        current_state = STOP_RCV_open;
                        printf("SET recived\n");
                    }
                    else if (byte == C_SET) current_state = C_RCV_open;
                    else current_state = START_open;
                    break;

                case C_RCV_open:
                    if (byte == FLAG) current_state = FLAG_RCV_open;
                    else if (byte == (A_T ^ C_SET)) current_state = BCC1_OK_open;
                    else current_state = START_open;
                    break;

                case BCC1_OK_open:
                    if (byte == FLAG)
                    {
                        current_state = START_open;
                        sendControlFrame(A_R, C_UA);
                        statistics.NumberFramesSent++;
                        
                    }
                    else current_state = START_open;
                    break;
                case STOP_RCV_open:
                    break;
                }
            }
        }
        break;
    default:
        break;
    }

    if(current_state != STOP_RCV_open) return -1;
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
    unsigned char *frame = (unsigned char *) malloc(framesize);
    frame[0] = FLAG;
    frame[1] = A_T;
    frame[2] = frameX ? 0x80 : 0x00;
    frame[3] = frame[1] ^ frame[2];

    unsigned char BCC2 = 0x00;
    for (int i = 0; i < bufSize; i++) {
        BCC2 ^= buf[i];
    }

    memcpy(frame+4, buf, bufSize);

    // Byte Stuffing data
    int j = 4;
    for (int i = 0; i < bufSize; i++) {
        
        if(buf[i] == FLAG || buf[i] == ESC)
        {
                framesize++;
                frame = realloc(frame, framesize);
                
                frame[j++] = ESC;
                if (buf[i] == FLAG) frame[j++] = 0x5E;
                else frame[j++] = 0x5D;
        }
        else frame[j++] = buf[i];   
    }

    // Byte Stuffing BCC2
    if (BCC2 == FLAG || BCC2 == ESC) 
    {
        framesize++;
        frame = realloc(frame, framesize);
        frame[j++] = ESC;

        if (BCC2 == FLAG) frame[j++] = 0x5E;
        else frame[j++] = 0x5D;
    } 
    else frame[j++] = BCC2;

    frame[j++] = FLAG;

    while (alarmCount < totalRetransmitions && current_state != STOP_RCV) {
            writeBytesSerialPort(frame, j);
            statistics.NumberFramesSent++;
            printf("Transmission: %d\n", alarmCount+1);

            alarm(timeout);
            alarmEnabled = TRUE;
            unsigned char cField = 0;

            while(current_state != STOP_RCV && alarmEnabled==TRUE) {
                if(readByteSerialPort(&byte)) {

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
                        else if (byte==C_REJ0 || byte==C_REJ1){
                            printf("Looks like we had problems, recived rejection byte 0x%02X\n", byte);
                            current_state = START;
                            alarmEnabled=FALSE;
                        }
                        else if ((frameX == 0x80 && byte == C_RR0) || (frameX == 0x00 && byte == C_RR1)){
                            cField = byte;
                            current_state = C_RCV;
                        }
                        else current_state = START;
                        break;

                    case C_RCV:
                        if (byte == FLAG) current_state = FLAG_RCV;
                        else if (byte == (A_R^cField)) current_state = BCC1_OK;
                        else current_state = START;
                        break;

                    case BCC1_OK:
                        if (byte == FLAG){
                            frameX = frameX ? 0x00 : 0x80;
                            current_state = STOP_RCV;
                        }
                        else {
                            current_state = START;
                        }
                            
                        break;
                    default:
                        break;
                    }
                }
            }

        }

    free(frame);
    if(current_state == STOP_RCV) return framesize;
    else {
        llclose(1);
        return -1;
    }
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    state cur_state = START;
    int i = 0;
    unsigned char controlField = 0x00;
    unsigned char bcc2_checker = 0x00;

    while( cur_state != STOP_RCV)
    {
        if(readByteSerialPort(&byte))
        {

            switch (cur_state) {
                case START:
                    if (byte == FLAG) cur_state = FLAG_RCV;
                    break;
                case FLAG_RCV:
                    if (byte == A_T) cur_state = A_RCV;
                    else if (byte == FLAG) break;
                    else cur_state = START;
                    break;  
                case A_RCV:
                    if (frameX == 0x00 && byte == 0x80){
                        sendControlFrame(A_R, C_RR0);
                        cur_state = START;
                    }
                    else if (frameX == 0x80 && byte == 0x00){
                        sendControlFrame(A_R, C_RR1);
                        cur_state = START;
                    }
                    else if ((frameX == 0x00 && byte == 0x00) || (frameX == 0x80 && byte == 0x80)){
                        cur_state = C_RCV;
                    }
                    else if (byte == FLAG) cur_state = FLAG_RCV;
                    else if (byte == C_DISC) {
                        sendControlFrame(A_R, C_DISC);
                        statistics.NumberFramesSent++;
                        return 0;
                    }
                    else cur_state = START;
                    break;
                case C_RCV:
                    if(byte == (A_T^frameX)) cur_state = DATA;
                    else if(byte == FLAG) cur_state = FLAG_RCV;
                    else cur_state = START;
                    break; 
                case DATA:
                    if (byte == ESC) cur_state = DATA_ESC;
                    else if (byte == FLAG){
                        unsigned char bcc2 = packet[i - 1];
                        packet[i-1] = 0x00;

                        bcc2_checker = bcc2_checker ^ bcc2;

                        if(bcc2 == bcc2_checker)
                        {
                            if (frameX) {
                                sendControlFrame(A_R, C_RR0);
                                statistics.NumberFramesSent++;
                            }
                            else {
                                sendControlFrame(A_R, C_RR1);
                                statistics.NumberFramesSent++;
                            }
                            frameX = frameX ? 0x00 : 0x80;
                            cur_state = STOP_RCV;
                            return (i - 1);
                        } 
                        else 
                        {
                            printf("Error: bbc2 checker fail :( \n");
                            if (controlField){
                                sendControlFrame(A_R, C_REJ1);
                                statistics.NumberFramesSent++;
                            }
                            else{
                                sendControlFrame(A_R, C_REJ0);
                                statistics.NumberFramesSent++;
                            }
                            return -1;
                        }
                            
                    }
                    else {
                        packet[i] = byte;
                        bcc2_checker = bcc2_checker ^ byte;
                        i++;
                    }
                    break;
                case DATA_ESC:
                    if(byte == 0x5e) {
                        bcc2_checker = bcc2_checker ^ FLAG;
                        packet[i] = FLAG;
                        i++;
                    }
                    else if (byte == 0x5d){
                        bcc2_checker = bcc2_checker ^ ESC;
                        packet[i] = ESC;
                        i++;
                    }
                    cur_state = DATA;
                    break;
                default:
                    printf("Error during llread\n");
                    break;
            }
        }
    }

    return -1;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    state cur_state = START;
    unsigned char byte;
    alarmCount = 0; 

    while(alarmCount < totalRetransmitions && cur_state != STOP_RCV){
        sendControlFrame(A_R, C_DISC);

        alarm(timeout);
        alarmEnabled = TRUE;
        while (cur_state != STOP_RCV && alarmEnabled==TRUE){
            if(readByteSerialPort(&byte)){
                switch (cur_state) {
                    case START:
                        if (byte == FLAG) cur_state = FLAG_RCV;
                        break;
                    case FLAG_RCV:
                        if (byte == A_R) cur_state = A_RCV;
                        else if (byte == FLAG) break;
                        else cur_state = START;
                        break;
                    case A_RCV:
                        if (byte == C_DISC) cur_state = C_RCV;
                        else if (byte == FLAG) cur_state = FLAG_RCV;
                        else cur_state = START;
                        break;
                    case C_RCV:
                        if (byte == (A_R ^ C_DISC)) cur_state = BCC1_OK;
                        else if (byte == FLAG) cur_state = FLAG_RCV;
                        else cur_state = START;
                        break;
                    case BCC1_OK:
                        if (byte == FLAG) {
                            printf("Close() sucessfull!\n");
                            sendControlFrame(A_T, C_UA);
                            cur_state = STOP_RCV;
                        } else cur_state = START;
                        break;
                    default:
                        printf("Oh no! Error during Close()\n");
                        break;
                }
            }
        }
    }

    if(showStatistics == 1 && cur_state == STOP_RCV){
        printf("=======STATISTICS=======\n");
        printf("Number of frames sent: %d\n", statistics.NumberFramesSent);
        printf("Number of retransmissions: %d\n", statistics.numberRetransmissions);
    }

    int clstat = closeSerialPort();
    return clstat;
}


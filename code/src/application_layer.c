// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    LinkLayer linkLayer;
    strcpy(linkLayer.serialPort, serialPort);
    linkLayer.role = strcmp(role, "tx") ? LlRx : LlTx;
    linkLayer.nRetransmissions = nTries;
    linkLayer.timeout = timeout;
    linkLayer.baudRate = baudRate;
    int BUF_SIZE = 5;
    unsigned char buf[5] = {0x01,0x7E,0x03,0x04,0x05};
    int total_char = 0;
    switch (linkLayer.role)
    {
    case LlTx:
        llopen(linkLayer);
        printf("finish llopen\n");

        total_char = llwrite(buf, BUF_SIZE);

        if(total_char == -1){
            printf("Llwrite error\n");
        }

        llclose(total_char);                
        break;
    case LlRx:
        unsigned char *packet = (unsigned char *)malloc(8);
        llopen(linkLayer);
        printf("finish llopen\n");
        
        total_char = llread(packet);

        if(total_char == -1){
            printf("Llread error\n");
        }

        for (int i =0; i<8; i++){
            printf("packet -- 0x%02X\n", packet[i]);
        }

        llclose(total_char);

        break;
    default:
        break;
    }
}

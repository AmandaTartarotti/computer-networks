// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


unsigned char *controlPacketBuilder (const char* filename, long filesize, int* size) {
 
    const int fname_space = strlen(filename);
    
    int fsize_space = 0;

    int filesize_copy = filesize;
    while (filesize_copy > 0) 
    {
        fsize_space++;
        filesize_copy /= 256;
    }

    *size = 5 + fsize_space + fname_space; // 5 pq é c + T + V + T2 + V2
    unsigned char *packet = (unsigned char*)malloc(*size);

    packet[0] = 1; //START
    packet[1] = 0; // T 
    packet[2] = fsize_space; // V

    int index = 3;
    for (int i = 0; i < fsize_space; i++)
    {
        packet[index] = (filesize >>  8 * i) & 0xFF;
        index++;
    }

    packet[index] = 1; // T2
    index++;
    packet[index] = fname_space; // V2
    index++;

    for (int i = 0; i < fname_space; i++)
    {
        packet[index] = filename[i];
        index++;
    }

    //Testando
    // printf("Conteúdo do control packet em bytes: %d\n", *size);
    // printf("Conteúdo:\n");
    // for (unsigned int i = 0; i < *size; i++) {
    //     printf("%02x \n", packet[i]);
    // }
    
    return packet;
}

long getFileSize(FILE *fptr) {
    fseek(fptr, 0, SEEK_END);
    long size = ftell(fptr);
    fseek(fptr, 0, SEEK_SET);

    //printf("size -- %lu", size);
    return size;
}

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
    unsigned char buf[5] = {0x01,0x7E,0x03,0x04,0x05}; //bcc2 igual a esc, bcc2 sai nos dados
    int total_char = 0;

    switch (linkLayer.role)
    {
    case LlTx:
        
        llopen(linkLayer);
        printf("finish llopen\n");

        FILE* fptr;
        fptr = fopen(filename, "rb");
        if (fptr == NULL) printf("File not found\n");

        long filesize = getFileSize(fptr);
        if (filesize == 0) printf("Empyt file found\n");

        int control_packet_size = 0;
        unsigned char *control_packet = controlPacketBuilder(filename,filesize, &control_packet_size);

        if (llwrite(control_packet, control_packet_size) == -1) printf("Llwrite Control Packet error\n");

        total_char = llwrite(buf, BUF_SIZE);

        if(total_char == -1){
            printf("Llwrite error\n");
        }

        llclose(total_char);                
        break;
    case LlRx:

        llopen(linkLayer);
        printf("finish llopen\n");

        unsigned char *controlpacket = (unsigned char *)malloc(MAX_PAYLOAD_SIZE);
        if (llread(controlpacket) == -1) printf("Llread Control Packet Error\n");

        unsigned long int file_size = 0;
        // unsigned char* file_name = controlPacketInfo(); to be implemented

        unsigned char *packet = (unsigned char *)malloc(8);

        
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

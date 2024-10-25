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

    if(llopen(linkLayer)<0){
        perror("Erro no open\n");
        exit(-1);
    }

    switch (linkLayer.role)
    {
    case LlTx:

        FILE* fptr;
        fptr = fopen(filename, "rb");
        if (fptr == NULL) printf("File not found\n");

        long filesize = getFileSize(fptr);
        if (filesize == 0) printf("Empyt file found\n");

        int control_packet_size = 0;
        unsigned char *control_packet = controlPacketBuilder(filename, filesize, &control_packet_size);

        if (llwrite(control_packet, control_packet_size) == -1) printf("Llwrite Control Packet error\n"); //sends control packet

        unsigned char sequence_number = 0;
        unsigned char* fileContent = (unsigned char*)malloc(sizeof(unsigned char) * filesize);
        fread(fileContent, sizeof(unsigned char), filesize, fptr); //Passa o que tem dentro da file para a variavel fileContent
        long int bytesLeft = filesize;

        while(bytesLeft >= 0){

            int datasize = 0;
            if(bytesLeft > (long int) MAX_PAYLOAD_SIZE){
                datasize = MAX_PAYLOAD_SIZE;
            }
            else {
                datasize = bytesLeft;
            }
            unsigned char* data = (unsigned char*) malloc(datasize);
            memcpy(data, fileContent, datasize);
            int packetsize;
            unsigned char* packet = dataPacketBuilder(sequence_number, data, datasize, &packetsize);

            if(llwrite(packet,packetsize) == -1){
                printf("Error in data packets\n");
                exit(-1);
            }


            bytesLeft -= (long int) MAX_PAYLOAD_SIZE;
            fileContent += datasize;
            sequence_number = (sequence_number + 1) % 255;
        }

        unsigned char *controlPacketEnd = getControlPacket(3, filename, filesize, &control_packet_size);
        if(llwrite(controlPacketEnd, control_packet_size) == -1) { 
            printf("Exit: error in end packet\n");
            exit(-1);
        }

        // total_char = llwrite(buf, BUF_SIZE);

        // if(total_char == -1){
        //     printf("Llwrite error\n");
        // }

        llclose(total_char);                
        break;



    case LlRx:

        unsigned char *packet = (unsigned char *)malloc(MAX_PAYLOAD_SIZE);
        int packetSize = -1;

        while (packetSize = llread(packet) <0); //Lê o packet de controle
        unsigned long int rcvFileSize = 0;
        unsigned char* name = controlPacketInfo(packet, packetSize, &rcvFileSize); //faz parse das informacoes do packet de controle

        FILE* rcvFile = fopen((char *) name, "wb+");

        while(TRUE){
            while((packetSize = llread(packet)) < 0){ //LÊ os packets de dados
                if(packetSize == 0) break;
                else if(packet[0] != 3){
                    unsigned char *buf = (unsigned char*)malloc(packetSize);
                    getDataPacket(packet, packetSize, buf); //faz parse das informacoes de dados
                    fwrite(buf, sizeof(unsigned char), packetSize-4, rcvFile);
                    free(buf);
                }
                else continue;
            }
        }

        fclose(rcvFile);
        break;

        // unsigned long int file_size = 0;
        // // unsigned char* file_name = controlPacketInfo(); to be implemented

        // unsigned char *packet = (unsigned char *)malloc(8);

        
        // total_char = llread(packet);

        // if(total_char == -1){
        //     printf("Llread error\n");
        // }

        // for (int i =0; i<8; i++){
        //     printf("packet -- 0x%02X\n", packet[i]);
        // }

        // llclose(total_char);

        break;

    default:
        exit(-1);
        break;
    }
}

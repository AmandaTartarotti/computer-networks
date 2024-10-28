// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define FILESIZE_TYPE 0
#define FILENAME_TYPE 1


unsigned char *controlPacketBuilder (const char* filename, int filesize, int* size) {
 
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
        packet[index + i] = filename[i];
    }

    //Testando
    // printf("Conteúdo do control packet em bytes: %d\n", *size);
    // printf("Conteúdo:\n");
    // for (unsigned int i = 0; i < *size; i++) {
    //     printf("%02x \n", packet[i]);
    // }
    
    return packet;
}

void controlPacketInfo(unsigned char* packet, int control_packet_size, int *fileSize, char *filename){

    int index = 2;

    if(packet[1] == FILESIZE_TYPE){
        int fsize_space = packet[2]; 

        int file_calculator = 0;
        for (int i = (fsize_space + 2); i >= 3 ; i--){
            file_calculator = file_calculator * 256 + (int)packet[i];
        }
        fileSize = file_calculator;
        // printf("fileSize -- %d\n", fileSize);
        index += fsize_space + 1;
    } else printf("Ops! Could not find fileSize Type\n");

    if(packet[index] == FILENAME_TYPE){
        index++;
        int fname_space = packet[index];

        index++;
        for(int i = 0; i < fname_space; i++)
            filename[i] = packet[index + i]; 
        // printf("fileName -- %s\n",filename);
    } else printf("Ops! Could not find fileName Type\n");
}

unsigned char * dataPacketBuilder(unsigned char sequence_number, unsigned char *data, int datasize, int *packetsize){
    
    *packetsize = 4 + datasize;

    unsigned char* packet = (unsigned char*)malloc(*packetsize);

    packet[0] = 2;
    packet[1] = sequence_number;
    packet[2] = datasize / 256;
    packet[3] = datasize % 256;
    memcpy(packet+4, data, datasize);

    return packet;
}

void dataPacketInfo(unsigned char *packet, int packetsize, unsigned char *buf) {
    if(packet[0] != 2) exit(1);
    memcpy(buf, packet+4, packetsize-4);
    //buffer += packetsize + 4;
}

int getFileSize(FILE *fptr) {
    fseek(fptr, 0, SEEK_END);
    int size = ftell(fptr);
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

        int filesize = getFileSize(fptr);
        if (filesize == 0) printf("Empyt file found\n");

        int control_packetSize = 0;
        unsigned char *control_packet = controlPacketBuilder(filename, filesize, &control_packetSize);

        if (llwrite(control_packet, control_packetSize) == -1) printf("Llwrite Control Packet error\n"); //sends control packet

        unsigned char sequence_number = 0;
        // unsigned char* fileContent = (unsigned char*)malloc(sizeof(unsigned char) * filesize);
        // fread(fileContent, sizeof(unsigned char), filesize, fptr); //Passa o que tem dentro da file para a variavel fileContent
        // long int bytesLeft = filesize;

        // while(bytesLeft >= 0){

        //     int datasize = 0;
        //     if(bytesLeft > (long int) MAX_PAYLOAD_SIZE){
        //         datasize = MAX_PAYLOAD_SIZE;
        //     }
        //     else {
        //         datasize = bytesLeft;
        //     }
        //     unsigned char* data = (unsigned char*) malloc(datasize);
        //     memcpy(data, fileContent, datasize);
        //     int packetsize;
        //     unsigned char* packet = dataPacketBuilder(sequence_number, data, datasize, &packetsize);

        //     if(llwrite(packet,packetsize) == -1){
        //         printf("Error in data packets\n");
        //         exit(-1);
        //     }


        //     bytesLeft -= (long int) MAX_PAYLOAD_SIZE;
        //     fileContent += datasize;
        //     sequence_number = (sequence_number + 1) % 255;
        // }

        // unsigned char *controlPacketEnd = getControlPacket(3, filename, filesize, &control_packet_size);
        // if(llwrite(controlPacketEnd, control_packet_size) == -1) { 
        //     printf("Exit: error in end packet\n");
        //     exit(-1);
        // }
        int packetsize;
        unsigned char* packet = dataPacketBuilder(sequence_number, buf, MAX_PAYLOAD_SIZE, &packetsize);
        total_char = llwrite(packet, packetsize);

        if(total_char == -1){
            printf("Llwrite error\n");
        }

        llclose(total_char);                
        break;



    case LlRx:

        unsigned char *packet_control = (unsigned char *)malloc(MAX_PAYLOAD_SIZE);
        int control_packet_size;

        control_packet_size = llread(packet_control); 
        int fileSize = 0;
        unsigned char fileName = "";
        controlPacketInfo(packet_control, control_packet_size, &fileSize, &fileName);
        // FILE* rcvFile = fopen((char *) name, "wb+");

        // while(TRUE){
        //     while((packetSize = llread(packet)) < 0){ //LÊ os packets de dados
        //         if(packetSize == 0) break;
        //         else if(packet[0] != 3){
        //             unsigned char *buf = (unsigned char*)malloc(packetSize);
        //             getDataPacket(packet, packetSize, buf); //faz parse das informacoes de dados
        //             fwrite(buf, sizeof(unsigned char), packetSize-4, rcvFile); //escreve os dados na file
        //             free(buf);
        //         }
        //         else continue;
        //     }
        // }

        // fclose(rcvFile);
        // break;

        int file_size = 0;
        unsigned char *packet_dummy = (unsigned char *)malloc(MAX_PAYLOAD_SIZE);
        int ps = llread(packet_dummy);

        printf("%d\n", ps);

        unsigned char buf[10];

        dataPacketInfo(packet_dummy, ps, buf);


        if(ps == -1){
            printf("Llread error\n");
        }

        for (int i =0; i<ps; i++){
            printf("data packet -- 0x%02X\n", buf[i]);
        }

        llclose(total_char);

        break;

    default:
        exit(-1);
        break;
    }
}

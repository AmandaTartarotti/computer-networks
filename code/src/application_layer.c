// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define FILESIZE_TYPE 0
#define FILENAME_TYPE 1


unsigned char *controlPacketBuilder (int c, const char* filename, int filesize, int* size) {
 
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

    packet[0] = c; //START
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

int controlPacketInfo(unsigned char* packet, int control_packet_size, char *filename){

    int index = 2;
    int file_calculator = 0;

    if(packet[1] == FILESIZE_TYPE){
        int fsize_space = packet[2]; 
        
        for (int i = (fsize_space + 2); i >= 3 ; i--){
            file_calculator = file_calculator * 256 + (int)packet[i];
        }
        index += fsize_space + 1;
    } else printf("Ops! Could not find fileSize Type\n");

    if(packet[index] == FILENAME_TYPE){
        index++;
        int fname_space = packet[index];

        index++;
        for(int i = 0; i < fname_space; i++)
            filename[i] = packet[index + i]; 
    } else printf("Ops! Could not find fileName Type\n");

    // printf("final fileSize during parsing ---%d\n", file_calculator);
    // printf("final fileName during parsing -- %s\n",filename);

    return file_calculator;
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
        unsigned char *control_packet = controlPacketBuilder(1, filename, filesize, &control_packetSize);

        if (llwrite(control_packet, control_packetSize) == -1) printf("Llwrite Control Packet error\n"); //sends control packet

        unsigned char sequence_number = 0;
        unsigned char* fileContent = (unsigned char*)malloc(sizeof(unsigned char) * filesize);
        fread(fileContent, sizeof(unsigned char), filesize, fptr); //Passa o que tem dentro da file para a variavel fileContent
        int bytesLeft = filesize;

        while(bytesLeft >= 0){

            int datasize = 0;
            printf("bytesLeft -- %d\n", bytesLeft);
            if(bytesLeft > MAX_PAYLOAD_SIZE){
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


            bytesLeft -= (int) MAX_PAYLOAD_SIZE;
            fileContent += datasize;
            sequence_number = (sequence_number + 1) % 255;
            printf("sequence num -- %u\n", sequence_number);
        }
        printf("Finish to transmit data file\n");
        control_packetSize = 0;
        unsigned char *controlPacketEnd = controlPacketBuilder(3, filename, filesize, &control_packetSize);
        total_char = llwrite(controlPacketEnd, control_packetSize);
        if(total_char == -1) { 
            printf("Exit: error in end packet\n");
            exit(-1);
        }

        // int packetsize;
        // unsigned char* packet = dataPacketBuilder(sequence_number, buf, MAX_PAYLOAD_SIZE, &packetsize);
        // total_char = llwrite(packet, packetsize);

        // if(total_char == -1){
        //     printf("Llwrite error\n");
        // }

        llclose(total_char);           
        break;


    case LlRx:

        unsigned char *packet_control = (unsigned char *)malloc(MAX_PAYLOAD_SIZE);
        int control_packet_size;

        control_packet_size = llread(packet_control); 
        int fileSize = 0;
        char fileName[100] = "";
        fileSize = controlPacketInfo(packet_control, control_packet_size, fileName);
        
        int bytes_read = 0;
        int packetSize = 0;
        unsigned char *file_buf = (unsigned char*)malloc(fileSize);

        if (file_buf == NULL) printf("Erro ao alocar memória.\n");

        unsigned char data_packet[MAX_PAYLOAD_SIZE];
        while (bytes_read < fileSize)
        {
            packetSize = llread(data_packet);
            if (data_packet[0] != 3)
            {
                for (int i = 0; i < packetSize - 4; i++)
                {
                    file_buf[bytes_read++] = data_packet[4 + i];
                }
                
            }
            printf("bytes_read --%d\n", bytes_read);
        }

        // Receive Control Packet - End
        unsigned char packet_control_end[MAX_PAYLOAD_SIZE];
        llread(packet_control_end); 


        // Escrever o arquivo
        FILE *rcvFile = fopen(filename, "w");
        if (rcvFile == NULL) printf("Erro ao abrir o arquivo.\n");
        if (fwrite(file_buf, fileSize, 1, rcvFile) != 1) printf("Erro ao escrever o arquivo.\n");
        fclose(rcvFile);
        free(file_buf);

        llclose(9); // numero qualquer por hora

        break;

    default:
        exit(-1);
        break;
    }
}

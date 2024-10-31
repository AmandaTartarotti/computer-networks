// Application layer protocol implementation
#include "application_layer.h"
#include "link_layer.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * Builds the Control Packet 
 * @param c control field (values: 1 – start; 3 – end)
 * @param filename name of the file to be transmitted.
 * @param filesize size of the file to be transmitted
 * @param size size of the generated packet
 * @return control packet data 
 */
unsigned char *controlPacketBuilder (int c, const char* filename, int filesize, int* size) {
    
    //Calculates the space needed to store the filename and filezise
    const int fname_space = strlen(filename);
    int fsize_space = 0;

    int filesize_copy = filesize;
    while (filesize_copy > 0) 
    {
        fsize_space++;
        filesize_copy /= 256;
    }

    // Dynamically allocates the packet space
    *size = 5 + fsize_space + fname_space;
    unsigned char *packet = (unsigned char*)malloc(*size);
    
    //Stores the data inside the packet
    int index = 0;
    packet[index++] = c;
    packet[index++] = 0; 
    packet[index++] = fsize_space;

    for (int i = 0; i < fsize_space; i++)
        packet[index++] = (filesize >>  8 * i) & 0xFF;

    packet[index++] = 1;
    packet[index++] = fname_space;


    for (int i = 0; i < fname_space; i++)
        packet[index + i] = filename[i];
    
    return packet;
}

/**
 * Get the Information from the given Control Packet
 * @param packet the Control Packet to be parsed
 * @param control_packet_size size of the Control Packet to be parsed
 * @param filename string where we will store the found file
 * @return the size of the found file
 */
int controlPacketInfo(unsigned char* packet, int control_packet_size, char *filename){

    int index = 1;
    int file_size = 0;

    //Gets the file's size
    if(packet[index++] == FILESIZE_TYPE){
        int fsize_space = packet[index++]; 
        for (int i = (fsize_space + 2); i >= 3 ; i--){
            file_size = file_size * 256 + (int)packet[i];
        }
        index += fsize_space;

    } else printf("Ops! Could not find fileSize Type\n");

    //Gets the file's name
    if(packet[index++] == FILENAME_TYPE){
        int fname_space = packet[index++];

        for(int i = 0; i < fname_space; i++)
            filename[i] = packet[index + i]; 
    } else printf("Ops! Could not find fileName Type\n");


    return file_size;
}

/**
 * Builds the Data Packet
 * @param sequence_number current sequence number (0-99)
 * @param data the data to be inserted in the data packet
 * @param datasize the size of the given data to be inserted in the data packet
 * @param packetsize size of the given data packet
 * @return packet data 
 */
unsigned char* dataPacketBuilder(unsigned char sequence_number, unsigned char* data, int datasize, int *packetsize){
    
    *packetsize = 4 + datasize;
    unsigned char* packet = (unsigned char*)malloc(*packetsize);

    packet[0] = 2;
    packet[1] = sequence_number;
    packet[2] = datasize / 256;
    packet[3] = datasize % 256;
    memcpy(packet+4, data, datasize);

    return packet;
}

/**
 * Gets the size of a given file
 * @param fptr pointer to a given file
 * @return the file's size
 */
int getFileSize(FILE *fptr) {
    fseek(fptr, 0, SEEK_END);
    int size = ftell(fptr);
    fseek(fptr, 0, SEEK_SET);
    return size;
}

/**
 * Handles the Reciver and Transmitter linkLayers
*/
void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    printf("\n");
    // Defines the linkLayer according to the given parameters
    LinkLayer linkLayer;
    strcpy(linkLayer.serialPort, serialPort);
    linkLayer.role = strcmp(role, "tx") ? LlRx : LlTx;
    linkLayer.nRetransmissions = nTries;
    linkLayer.timeout = timeout;
    linkLayer.baudRate = baudRate;

    // Sends and receives the control frame (SET/UA)
    if(llopen(linkLayer) < 0){
        perror("Erro during llopen!\n");
        exit(-1);
    }

    // Definition of usefull variables 
    FILE* fptr; 
    unsigned char* data;
    unsigned char* packet;
    unsigned char *controlPacketEnd;
    unsigned char* control_packet;
    unsigned char* fileContent;
    unsigned char* fileBuf;
    unsigned char sequence_number = 0;
    int bytesLeft = 0;
    int bytes_read = 0;
    int file_size = 0;
    int datasize = 0;
    int packetsize = 0;
    int control_packet_size = 0;


    switch (linkLayer.role)
    {
    case LlTx:

        //Opens the given file and get the file's info
        fptr = fopen(filename, "rb");
        if (fptr == NULL) printf("File not found\n");
        file_size = getFileSize(fptr);
        if (file_size == 0) printf("Empyt file found\n");

        //Builds the Control Packet with the file's info and sends it
        control_packet = controlPacketBuilder(1, filename, file_size, &control_packet_size);
        if (llwrite(control_packet, control_packet_size) == -1) {
            printf("Llwrite Control Packet error\n");
            exit(-1);
        }
        printf("Control packet sent!\n");

        fileContent = (unsigned char*)malloc(sizeof(unsigned char) * file_size);
        if (fileContent == NULL) printf("Erro ao alocar memória.\n");
        fread(fileContent, sizeof(unsigned char), file_size, fptr);
        printf("\n");
        
        //While still has bytes left, sends it through a data packet
        bytesLeft = file_size;
        while(bytesLeft >= 0){

            datasize = 0;
            printf("\nData Packet number ------------- %u\n", sequence_number);
            printf("BytesLeft to be sent ----------- %d\n", bytesLeft);
            if(bytesLeft > MAX_PAYLOAD_SIZE){
                datasize = MAX_PAYLOAD_SIZE;
            }
            else datasize = bytesLeft;

            data = (unsigned char*) malloc(datasize);
            memcpy(data, fileContent, datasize);
            packet = dataPacketBuilder(sequence_number, data, datasize, &packetsize);

            if(llwrite(packet,packetsize) == -1){
                printf("Data packets timeout!\n");
                exit(-1);
            }

            bytesLeft -= (int) MAX_PAYLOAD_SIZE;
            fileContent += datasize;
            sequence_number = (sequence_number + 1) % 255;
        }

        printf("\n>>>>> Finish to transmit data file <<<<<<\n\n");

        //Sends the Control Packet - End
        controlPacketEnd = controlPacketBuilder(3, filename, file_size, &control_packet_size);
        if(llwrite(controlPacketEnd, control_packet_size) == -1) { 
            printf("Exit: error in end packet\n");
            exit(-1);
        }

        if(llclose(1) == -1) printf("Error during llclose()\n");           
        break;


    case LlRx:

        //Recives the Control Packet
        control_packet = (unsigned char *)malloc(MAX_PAYLOAD_SIZE);
        control_packet_size = llread(control_packet); 
        char fileName[100] = "";
        file_size = controlPacketInfo(control_packet, control_packet_size, fileName);
        
        //Space to save the content from File
        fileBuf = (unsigned char*)malloc(file_size);
        if (fileBuf == NULL) printf("Erro ao alocar memória.\n");

        //Recives the bytes from the data packet
        unsigned char data_packet[MAX_PAYLOAD_SIZE];
        while (bytes_read < file_size)
        {
            packetsize = llread(data_packet);
            if (data_packet[0] != 3 && packetsize != -1)
            {
                for (int i = 0; i < packetsize - 4; i++)
                    fileBuf[bytes_read++] = data_packet[4 + i];
                printf("Bytes read ----------- %d\n", bytes_read);
            }
        }

        // Receives Control Packet - End
        unsigned char packet_control_end[MAX_PAYLOAD_SIZE];
        if(llread(packet_control_end) == -1) printf("Error reading final control packet!\n"); 


        // Writes the penguin-recived file
        FILE *rcvFile = fopen(filename, "wb");
        if (rcvFile == NULL) printf("Error opening the file.\n");
        if (fwrite(fileBuf, file_size, 1, rcvFile) != 1) printf("Error writing file.\n");
        fclose(rcvFile);

        //Free memory
        free(fileBuf);
        free(control_packet);

        if(llclose(1) == -1) printf("Error during llclose()\n"); 

        break;

    default:
        exit(-1);
        break;
    }
}

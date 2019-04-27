#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <sys/stat.h>
#include <libgen.h>
#include <pthread.h>

void* handle_send(void *);


#define ERR_EXIT(m) \
        do \
        { \
                perror(m); \
                exit(EXIT_FAILURE); \
        } while(0)

#define BUFFER_SIZE 1024
#define SERVER_PORT 10108

char *filepath;

int main(int argc, char *argv[])
{
    int sock;
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        ERR_EXIT("socket error");

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVER_PORT);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        ERR_EXIT("bind error");

    filepath = argv[1];
    char buffer[10] = {};
    while(1) {
        struct sockaddr_in client_addr;
        socklen_t length = sizeof(client_addr);
        recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &length);
        sendto(sock, "OK", 2, 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
        pthread_t thread;
        pthread_create(&thread, NULL, handle_send, (void*)&client_addr);
    }

    close(sock);
	
}

void* handle_send(void *data) {

    struct sockaddr_in client_addr = *((struct sockaddr_in *)data);

    int sock;
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        ERR_EXIT("socket error");

    int bytes;

    int ret, filesize;
    char sendbuf[BUFFER_SIZE] = {0};
    char recvbuf[16] = {0};
    
    FILE *fp;
    struct stat filest;

    //open the file, read byte
    fp = fopen(filepath, "rb");
    if(fp == NULL) {
        ERR_EXIT("open file error");
    }

    //check file size
    stat(filepath, &filest);
    filesize = filest.st_size;

    //send file name, let receiver knows file type
    char *filename = basename(filepath);
    strcpy(sendbuf, filename);
    
    sendto(sock, sendbuf, strlen(sendbuf), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
    

    printf("client form %s : %d is processing...\n", inet_ntoa(client_addr.sin_addr), htons(client_addr.sin_port));

    while (!feof(fp))
    {
        bytes = fread(sendbuf, sizeof(char), sizeof(sendbuf), fp);

        //sned bytes
        bytes = sendto(sock, sendbuf, bytes, 0, (struct sockaddr *)&client_addr, sizeof(client_addr));

        memset(sendbuf, 0, sizeof(sendbuf));
        memset(recvbuf, 0, sizeof(recvbuf));
    }
    //let receiver knows the end of send
    sendto(sock, sendbuf, 0, 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
    printf("\nfinish request form %s : %d\n\n", inet_ntoa(client_addr.sin_addr), htons(client_addr.sin_port));
}


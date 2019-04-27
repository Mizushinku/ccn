#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <netdb.h>
#include <sys/stat.h>
#include <libgen.h>
#include <sys/time.h>

#define ERR_EXIT(m) \
        do \
        { \
                perror(m); \
                exit(EXIT_FAILURE); \
        } while(0)

#define BUFFER_SIZE 1024
#define SERVER_PORT 10108

int main(int argc, char *argv[])
{
    int sock, bytes, scnt = 0;
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        ERR_EXIT("socket");

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
        ERR_EXIT("set socket timeout opt error");

    struct sockaddr_in servaddr;
	struct hostent *server;

	server = gethostbyname("localhost");
	if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
	bcopy((char *)server->h_addr_list[0], 
         (char *)&servaddr.sin_addr.s_addr,
         server->h_length);
    servaddr.sin_port = htons(SERVER_PORT);

    printf("waitting for server reply...\n");

    char recvbuf[BUFFER_SIZE] = {0};
    struct sockaddr_in peeraddr;
    socklen_t peerlen;
    while(1) {
        sendto(sock, "connect", 7, 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
        recvfrom(sock, recvbuf, sizeof(recvbuf), 0, (struct sockaddr *)&peeraddr, &peerlen);
        if(strcmp(recvbuf, "OK") == 0)
            break;
    }

    printf("server reply\n");
    
    
    int n = 0;
    char tmp[100] = {},  filename[100] = "out_";
    if(argc == 2) {
        strcat(filename, argv[1]);
        strcat(filename, "_");
    }
    FILE *fp;
    //wait for sender sends you file name
    do {
        peerlen = sizeof(peeraddr);
        memset(recvbuf, 0, sizeof(recvbuf));
        n = recvfrom(sock, recvbuf, sizeof(recvbuf), 0,
                    (struct sockaddr *)&peeraddr, &peerlen);
        if(n <= 0) continue;    
        strcpy(tmp, recvbuf);
        strcat(filename, tmp);
    }while(n == 0);
    printf("\nfile name = %s\n\nprocessing...\n", filename);
    

    fp = fopen(filename, "wb");
    if(fp == NULL) {
        ERR_EXIT("error open file");
    }

    while (1)
    {

        peerlen = sizeof(peeraddr);
        memset(recvbuf, 0, sizeof(recvbuf));
        n = recvfrom(sock, recvbuf, sizeof(recvbuf), 0,
                     (struct sockaddr *)&peeraddr, &peerlen);
        if (n == -1)
        {

            if (errno == EINTR)
                continue;

            ERR_EXIT("recvfrom error");
        }
        else if(n > 0)
        {
            fwrite(recvbuf, sizeof(char), n, fp);
        } else if(n == 0) {
            printf("\nfinish\n\n");
            break;
        }
    }
    close(sock);
}

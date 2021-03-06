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

#define BUFFER_SIZE 10240
#define SERVER_PORT 10108

int main(int argc, char *argv[])
{   
    //initialize socket
    int sock, bytes, scnt = 0;
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        ERR_EXIT("socket");

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

    //set recv timeout
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
        ERR_EXIT("set socket timeout opt error");

    printf("waitting for server reply...\n");

    //each time interval send connect request to server, waiting for reply
    unsigned char recvbuf[BUFFER_SIZE] = {0};
    struct sockaddr_in peeraddr;
    socklen_t peerlen;
    while(1) {
        peerlen = sizeof(peeraddr);
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
    
    //open file
    fp = fopen(filename, "wb");
    if(fp == NULL) {
        ERR_EXIT("error open file");
    }

    //reset recv timeout
    int packnum, packcnt = 0;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
        ERR_EXIT("set socket timeout opt error");
    while (1)
    {

        //receeive n bytes
        peerlen = sizeof(peeraddr);
        memset(recvbuf, 0, sizeof(recvbuf));
        n = recvfrom(sock, recvbuf, sizeof(recvbuf), 0,
                     (struct sockaddr *)&peeraddr, &peerlen);

        
        if (n == -1)
        {
            if (errno == EINTR)
                continue;
            if(errno == EWOULDBLOCK) {
                //recvfrom time out
                printf("recvfrom time out\n");
                break;
            }

            ERR_EXIT("recvfrom error");
        }
        else if(n > 0)
        {
            //get packet number
            packnum = 0;
            for(int i = 0; i < 4; ++i) {
		        packnum += recvbuf[i] << (24 - i*8);
	        }
            //printf("receive pack:%d from server\n", packnum);
            
            ++packcnt;
            fwrite(recvbuf+4, sizeof(char), n-4, fp);
            //reply
            sendto(sock, "OK", 2, 0, (struct sockaddr *)&peeraddr, peerlen);
        } else if(n == 0) {
            //show the packet loss rate
            float rate = 0;
            if(packnum - packcnt != 0) {
                rate = ((float)(packnum - packcnt))/packnum * 100;
            }
            printf("\ntotol pack = %d\npacket loss rate = %.2f%%\n\n", packnum, rate);
            break;
        }
    }
    printf("finish\n\n");
    fclose(fp);
    
    close(sock);
}

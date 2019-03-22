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
#include <time.h>

void tcp_send(char *, char *, char *);
void tcp_recv(char *, char *);
void udp_send(char *, char *, char *);
void udp_recv(char *, char *);

#define ERR_EXIT(m) \
        do \
        { \
                perror(m); \
                exit(EXIT_FAILURE); \
        } while(0)

#define BUFFER_SIZE 10240


int main(int argc, char *argv[])
{
	//specify the method
	if(strcmp(argv[1], "tcp") == 0) {
		if(strcmp(argv[2], "send") == 0) {
			tcp_send(argv[3], argv[4], argv[5]);
		} else if(strcmp(argv[2], "recv") == 0){
			tcp_recv(argv[3], argv[4]);
		} else {
			ERR_EXIT("input string error");
		}
	} else if(strcmp(argv[1], "udp") == 0) {
		if(strcmp(argv[2], "send") == 0){
			udp_send(argv[3], argv[4], argv[5]);
		} else if(strcmp(argv[2], "recv") == 0){
			udp_recv(argv[3], argv[4]);
		}else {
			ERR_EXIT("input string error");
		}
	} else {
		ERR_EXIT("input string error");
	}

	return 0;
}

void tcp_send(char *ip, char *port, char *filepath)
{
	//printf("\nmethod = tcp_send\nip = %s\nport = %s\nfile path = %s\n\n", ip, port, filepath);
	int sockfd, portno, n, filesize, bytes, scnt = 0;
    float bar = 0.0;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    FILE *fp;
    struct stat filest;
    time_t timep;
    struct tm *tmptr;

    //10KB buffer
    char buffer[BUFFER_SIZE];
    
    //get port number from input
    portno = atoi(port);

    //get socket file descriptor
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        ERR_EXIT("ERROR opening socket");

    //get the target ip address
    server = gethostbyname(ip);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    //init the sockaddr_in
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);

    //get the ip address form hostent
    //h_addr_list can contain many ip address. use only the first element here
    bcopy((char *)server->h_addr_list[0], 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    
    //try to connect
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        ERR_EXIT("ERROR connecting");

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
    write(sockfd, filename, strlen(filename));
    //wait for reply
    read(sockfd,buffer,2);

    printf("processing...\n");
    //transfer the file
    while(!feof(fp)) {
        //read file data to the buffer
        bytes = fread(buffer, sizeof(char), sizeof(buffer), fp);
        //write to the receiver
        bytes = write(sockfd,buffer,bytes);
        if (bytes < 0) 
            ERR_EXIT("ERROR writing to socket");
        
        //waiting for reply
        n = read(sockfd,buffer,2);
        if (n < 0) 
            ERR_EXIT("ERROR reading from socket");
        if(strncmp(buffer, "OK", 2) != 0) {
            ERR_EXIT("ERROR sending file");
        }

        //check transfer status
        //update the status when get a reply
        scnt += bytes;
        float status = (float)scnt/filesize * 100.0;
        while(status >= bar + 5.0) {
            bar += 5.0;
            time(&timep);
            tmptr = localtime(&timep);
            printf("%.0f%% ", bar);
            printf("%d/%d/%d ", 1900+tmptr->tm_year, 1+tmptr->tm_mon, tmptr->tm_mday);
            printf("%d:%d:%d\n", tmptr->tm_hour, tmptr->tm_min, tmptr->tm_sec);
        }
    }
    printf("\nfinish\n\n");

    close(sockfd);
}

void tcp_recv(char *ip, char * port)
{
	//printf("\nmethod = tcp_recv\nip = %s\nport = %s\n\n", ip, port);
	int sockfd, newsockfd, portno, bytes;
    socklen_t clilen;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    FILE *fp;

    //get socket file descriptor
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
       ERR_EXIT("ERROR opening socket");

    //init the sockaddr_in
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(port);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    //let the socket process not wait the WAIT_TIME on the port
    int on = 1;
    if((setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) < 0) {
        ERR_EXIT("setsocketopt error\n");
    }

    //bind the socket
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
             sizeof(serv_addr)) < 0)
             ERR_EXIT("ERROR on binding");

    //listen the socket, waiting for someone wants to connect
    listen(sockfd,5);
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd,
              (struct sockaddr *) &cli_addr,
                &clilen);
    if (newsockfd < 0)
         ERR_EXIT("ERROR on accept");
    
    bzero(buffer,sizeof(buffer));

    //get file name
    char tmp[100] = {},  filename[100] = "out_";
    read(newsockfd, tmp,sizeof(tmp));
    //reply
    write(newsockfd,"OK",2);
    strcat(filename, tmp);
    printf("\nfile name = %s\n\nprocessing...\n", filename);

    //create file
    fp = fopen(filename, "wb");
    if(fp == NULL) {
        ERR_EXIT("error open file");
    }

    //receive data fome sender
    while(1) {
        bytes = read(newsockfd,buffer,sizeof(buffer));
        if (bytes < 0) ERR_EXIT("ERROR reading from socket");


        //end of transfer
        if(bytes == 0) {
            printf("finish\n\n");
            break;
        }

        bytes = fwrite(buffer, sizeof(char), bytes, fp);

        //reply OK
        n = write(newsockfd,"OK",2);
        if (n < 0) ERR_EXIT("ERROR writing to socket");
    }
    
    close(newsockfd);
    close(sockfd);
}

void udp_send(char *ip, char *port, char *filepath)
{
	//printf("\nmethod = udp_send\nip = %s\nport = %s\nfile path = %s\n\n", ip, port, filepath);
	int sock;
    if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
        ERR_EXIT("socket");

    struct sockaddr_in servaddr;
	struct hostent *server;

	server = gethostbyname(ip);
	if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
	bcopy((char *)server->h_addr_list[0], 
         (char *)&servaddr.sin_addr.s_addr,
         server->h_length);
    servaddr.sin_port = htons(atoi(port));

    int ret, filesize;
    char sendbuf[BUFFER_SIZE] = {0};
    char recvbuf[16] = {0};
    
    FILE *fp;
    struct stat filest;
    time_t timep;
    struct tm *tmptr;

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
    sendto(sock, sendbuf, strlen(sendbuf), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
    

    printf("processing...\n");

    while (!feof(fp))
    {

        sendto(sock, sendbuf, strlen(sendbuf), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));

        ret = recvfrom(sock, recvbuf, sizeof(recvbuf), 0, NULL, NULL);
        if (ret == -1)
        {
            if (errno == EINTR)
                continue;
            ERR_EXIT("recvfrom");
        }

		fputs("get the msg :", stdout);
        fputs(recvbuf, stdout);
        memset(sendbuf, 0, sizeof(sendbuf));
        memset(recvbuf, 0, sizeof(recvbuf));
    }

    close(sock);
}

void udp_recv(char *ip, char * port)
{
	//printf("\nmethod = udp_recv\nip = %s\nport = %s\n\n", ip, port);
	int sock;
    if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
        ERR_EXIT("socket error");

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(port));
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        ERR_EXIT("bind error");

	char recvbuf[BUFFER_SIZE] = {0};
    struct sockaddr_in peeraddr;
    socklen_t peerlen;
    int n = 0;
    char tmp[100] = {},  filename[100] = "out_";
    FILE *fp;

    do {
        peerlen = sizeof(peeraddr);
        memset(recvbuf, 0, sizeof(recvbuf));
        n = recvfrom(sock, recvbuf, sizeof(recvbuf), 0,
                    (struct sockaddr *)&peeraddr, &peerlen);
        strcpy(tmp, recvbuf);
        strcat(filename, tmp);
        printf("\nfile name = %s\n\n", filename);
    }while(n == 0);

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

            fputs(recvbuf, stdout);
            sendto(sock, "OK", 2, 0,
                   (struct sockaddr *)&peeraddr, peerlen);
        }
    }
    close(sock);
}

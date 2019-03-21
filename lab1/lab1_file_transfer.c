#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <netdb.h>

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
	int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];
    
    portno = atoi(port);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        ERR_EXIT("ERROR opening socket");
    server = gethostbyname(ip);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;

    bcopy((char *)server->h_addr_list[0], 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        ERR_EXIT("ERROR connecting");
    //printf("Please enter the message: ");
    //bzero(buffer,256);
    //fgets(buffer,255,stdin);
	strcpy(buffer, filepath);
    n = write(sockfd,buffer,strlen(buffer));
    if (n < 0) 
         ERR_EXIT("ERROR writing to socket");
    bzero(buffer,256);
    n = read(sockfd,buffer,255);
    if (n < 0) 
         ERR_EXIT("ERROR reading from socket");
    printf("%s\n",buffer);
    close(sockfd);
}

void tcp_recv(char *ip, char * port)
{
	//printf("\nmethod = tcp_recv\nip = %s\nport = %s\n\n", ip, port);
	int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
       ERR_EXIT("ERROR opening socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(port);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    int on = 1;
    if((setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) < 0) {
        ERR_EXIT("setsocketopt error\n");
    }

    if (bind(sockfd, (struct sockaddr *) &serv_addr,
             sizeof(serv_addr)) < 0)
             ERR_EXIT("ERROR on binding");
    listen(sockfd,5);
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd,
              (struct sockaddr *) &cli_addr,
                &clilen);
    if (newsockfd < 0)
         ERR_EXIT("ERROR on accept");
    bzero(buffer,256);
    n = read(newsockfd,buffer,255);
    if (n < 0) ERR_EXIT("ERROR reading from socket");
    printf("Here is the message: %s\n",buffer);
    n = write(newsockfd,"I got your message",18);
    if (n < 0) ERR_EXIT("ERROR writing to socket");
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

    int ret;
    char sendbuf[1024] = {0};
    char recvbuf[1024] = {0};
    while (fgets(sendbuf, sizeof(sendbuf), stdin) != NULL)
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

	char recvbuf[1024] = {0};
    struct sockaddr_in peeraddr;
    socklen_t peerlen;
    int n;

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
            sendto(sock, recvbuf, n, 0,
                   (struct sockaddr *)&peeraddr, peerlen);
        }
    }
    close(sock);
}

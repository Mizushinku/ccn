/* Receiver/client multicast Datagram example. */
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>

#define BUFFER_SIZE 10240
 
struct sockaddr_in localSock;
struct ip_mreq group;
int sd;
int datalen;
unsigned char databuf[BUFFER_SIZE];
 
int main(int argc, char *argv[])
{
/* Create a datagram socket on which to receive. */
	sd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sd < 0)
	{
		perror("Opening datagram socket error");
		exit(1);
	}
	else
	printf("Opening datagram socket....OK.\n");
		 
	/* Enable SO_REUSEADDR to allow multiple instances of this */
	/* application to receive copies of the multicast datagrams. */
	{
		int reuse = 1;
	if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0)
	{
		perror("Setting SO_REUSEADDR error");
		close(sd);
		exit(1);
	}
	else
		printf("Setting SO_REUSEADDR...OK.\n");
	}
	 
	/* Bind to the proper port number with the IP address */
	/* specified as INADDR_ANY. */
	memset((char *) &localSock, 0, sizeof(localSock));
	localSock.sin_family = AF_INET;
	localSock.sin_port = htons(4321);
	localSock.sin_addr.s_addr = INADDR_ANY;
	if(bind(sd, (struct sockaddr*)&localSock, sizeof(localSock)))
	{
		perror("Binding datagram socket error");
		close(sd);
		exit(1);
	}
	else
		printf("Binding datagram socket...OK.\n");
	 
	/* Join the multicast group 226.1.1.1 on the local 203.106.93.94 */
	/* interface. Note that this IP_ADD_MEMBERSHIP option must be */
	/* called for each local interface over which the multicast */
	/* datagrams are to be received. */
	group.imr_multiaddr.s_addr = inet_addr("226.1.1.1");
	group.imr_interface.s_addr = inet_addr("10.0.2.15");
	if(setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group, sizeof(group)) < 0)
	{
		perror("Adding multicast group error");
		close(sd);
		exit(1);
	}
	else
		printf("Adding multicast group...OK.\n");
	 
	/* Read from the socket. */
	FILE *fp;
	char filepath[100] = {"out_"};
	if(argc > 1) {
		strcat(filepath, argv[1]);
	}
	fp = fopen(filepath, "wb");
	if(fp == NULL) {
		perror("open file error");
		exit(1);
	}
	
	printf("Waiting for server sending a file...\n");
	int bytes = 0;
	struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
	int first_recv = 1;
	while(1) {
		bytes = read(sd, databuf, sizeof(databuf));
		
		if(bytes < 0) {
			if(errno == EINTR) continue;
			if(errno == EWOULDBLOCK) {
				printf("read time out...break read loop\n");
				break;
			}
			perror("Reading datagram message error");
			close(sd);
			exit(1);
		}
		else if(bytes > 0) {
			fwrite(databuf, sizeof(char), bytes, fp);
			if(first_recv) {
				if(setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
					perror("set socket timeout opt error");
					close(sd);
					exit(1);
				}
				first_recv = 0;
			}
		} else {
			break;
		}
	}
	printf("\nReceiving a file...finish\n\n");

	fclose(fp);
	close(sd);
	return 0;
}

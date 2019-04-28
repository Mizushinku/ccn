/* Send Multicast Datagram code example. */
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <liquid/liquid.h>
#include <math.h>

#define BUFFER_SIZE 8192
#define ENCODE_BUF_SIZE 14336

struct in_addr localInterface;
struct sockaddr_in groupSock;
int sd;
unsigned char databuf[BUFFER_SIZE] = {};
unsigned char encode_buf[ENCODE_BUF_SIZE] = {};
int datalen = sizeof(databuf);
char *filepath;
 
int main (int argc, char *argv[ ])
{
/* Create a datagram socket on which to send. */
	sd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sd < 0)
	{
	  perror("Opening datagram socket error");
	  exit(1);
	}
	else
	  printf("Opening the datagram socket...OK.\n");
	 
	/* Initialize the group sockaddr structure with a */
	/* group address of 225.1.1.1 and port 5555. */
	memset((char *) &groupSock, 0, sizeof(groupSock));
	groupSock.sin_family = AF_INET;
	groupSock.sin_addr.s_addr = inet_addr("226.1.1.1");
	groupSock.sin_port = htons(4321);
	 
	// Disable loopback so you do not receive your own datagrams.
	/*
	char loopch = 0;
	if(setsockopt(sd, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&loopch, sizeof(loopch)) < 0) {
		perror("Setting IP_MULTICAST_LOOP error");
		close(sd);
		exit(1);
	} else
	printf("Disabling the loopback...OK.\n");
	*/
	 
	/* Set local interface for outbound multicast datagrams. */
	/* The IP address specified must be associated with a local, */
	/* multicast capable interface. */
	localInterface.s_addr = inet_addr("10.0.2.15");
	if(setsockopt(sd, IPPROTO_IP, IP_MULTICAST_IF, (char *)&localInterface, sizeof(localInterface)) < 0)
	{
	  perror("Setting local interface error");
	  exit(1);
	}
	else
	  printf("Setting the local interface...OK\n");
	/* Send a message to the multicast group specified by the*/
	/* groupSock sockaddr structure. */
	/*int datalen = BUFFER_SIZE;*/

	FILE *fp;
	filepath = argv[1];
	fp = fopen(filepath, "rb");
	if(fp == NULL) {
		perror("open file error");
		exit(1);
	}

	printf("Sending a file to group...processing\n");
	int bytes = 0, packnum = 0;

	//FEC, using Hamming(7,4)
	fec hm74;
	if(argc == 3) {
		fec_scheme fs = LIQUID_FEC_HAMMING74;
		hm74 = fec_create(fs, NULL);
		fec_print(hm74);
	}

	while(!feof(fp)) {
		++packnum;
		for(int i = 0; i < 4; ++i) {
			databuf[i] = (packnum >> (24 - i*8));
		}
		bytes = fread(databuf+4, sizeof(unsigned char), sizeof(databuf)-4, fp);

		if(argc == 3) {
			fec_encode(hm74, bytes+4, databuf, encode_buf);
			int encode_len = ceil((bytes + 4) * 7.0 / 4.0);

			if(sendto(sd, encode_buf, encode_len, 0, (struct sockaddr*)&groupSock, sizeof(groupSock)) < 0) {
				perror("Sending datagram message error");
			}
		} else {
			if(sendto(sd, databuf, bytes+4, 0, (struct sockaddr*)&groupSock, sizeof(groupSock)) < 0) {
				perror("Sending datagram message error");
			}
		}
		memset(databuf, 0, sizeof(databuf));
	}
	sendto(sd, "", 0, 0, (struct sockaddr*)&groupSock, sizeof(groupSock));
	printf("\nSending a file (%d packets)...finish\n\n", packnum);
	//sendto(sd, "OK", 2, 0, (struct sockaddr*)&groupSock, sizeof(groupSock));
	
	 
	/* Try the re-read from the socket if the loopback is not disable
	if(read(sd, databuf, datalen) < 0)
	{
	perror("Reading datagram message error\n");
	close(sd);
	exit(1);
	}
	else
	{
	printf("Reading datagram message from client...OK\n");
	printf("The message is: %s\n", databuf);
	}
	*/
	fclose(fp);
	close(sd);
	return 0;
}
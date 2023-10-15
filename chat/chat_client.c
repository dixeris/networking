#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "socketheader.h"
#define MAXDATASIZE 100


void *get_in_addr(struct sockaddr *sa) {
	if(sa->sa_family == AF_INET)  {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
}

int main(int argc, char* argv[]) {
	int sockfd, numbytes; 
	char buf[MAXDATASIZE];
	char s[INET6_ADDRSTRLEN];
	struct addrinfo hints, *servinfo;
	int rv;
	
	memset(&hints,0,sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if((rv=getaddrinfo(argv[1],"3396",&hints,&servinfo)) != 0) {
		fprintf(stderr,"getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}
	if((sockfd=socket(servinfo->ai_family,servinfo->ai_socktype,servinfo->ai_protocol)) == -1) {
		perror("socket");
	}
	if(connect(sockfd,servinfo->ai_addr,servinfo->ai_addrlen) == -1) {
		close(sockfd);
		perror("connect");
	}
	inet_ntop(servinfo->ai_family, get_in_addr((struct sockaddr*)servinfo->ai_addr), s, sizeof(s));
	printf("client connect to: %s\n", s);
	freeaddrinfo(servinfo);
	char* data = malloc(MAXDATASIZE);
	while(fgets(data,MAXDATASIZE,stdin) != NULL) {
		write(sockfd,data,MAXDATASIZE);

		if(read(sockfd,buf,MAXDATASIZE) == 0) {
			close(sockfd);
			printf("server terminteed");
		}
		if(strncmp(buf,"quit",4) == 0) { close(sockfd); printf("exiting...\n"); break; }
		printf("received: %s\n", buf);
	}
				


return 0;
	
	}


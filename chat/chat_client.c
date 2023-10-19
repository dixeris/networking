#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <poll.h>
#include "socketheader.h"
#define MAXDATASIZE 100


void *get_in_addr(struct sockaddr *sa) {
	if(sa->sa_family == AF_INET)  {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
}

void *receiver(void* void_sockfd) {
	char buf[256];
	struct pollfd pfds;
	int sockfd;
	sockfd = *((int*)void_sockfd);
	
	pfds.fd = sockfd;
	pfds.events = POLLIN;

	for(;;) {
	int poll_count = poll(&pfds,1,-1);
	if(poll_count > 0) { 
		if(pfds.revents == POLLIN) {	
			if((read(sockfd,buf,256)) == -1) {
				perror("read");
				close(sockfd);
				}
			else {
				printf("received: %s\n", buf);
				}
			}
		}
	}
}


int main(int argc, char* argv[]) {
	int sockfd, numbytes; 
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


	pthread_t thread;
	pthread_create(&thread, NULL, receiver, (void*)&sockfd);


	char* data = malloc(256);

	while(fgets(data,MAXDATASIZE,stdin) != NULL) {
		write(sockfd,data,MAXDATASIZE);

		if((strncmp(data,"quit",4) == 0) || strncmp(data,"exit",4) == 0) { close(sockfd); printf("exiting...\n"); free(data); break; }
	}
return 0;
}
	
	



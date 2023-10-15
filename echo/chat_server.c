#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "socketheader.h"

#define MAXDATASIZE 100


void *get_in_addr(struct sockaddr *sa) {
	if(sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
}

void *clientHandler(void* void_sockfd) {
	int new_fd;
	struct sockaddr_storage theiraddr;
	char s[INET6_ADDRSTRLEN];
	socklen_t sin_size;
	int sockfd  = *(int*)void_sockfd;
	char buf[MAXDATASIZE];
	int strlen;

	while (1) {
	
		sin_size = sizeof(theiraddr);
		new_fd = accept(sockfd, (struct sockaddr*)&theiraddr, &sin_size);
		if(new_fd == -1) {
			perror("accept");					
			continue;
			} 
		
		inet_ntop(theiraddr.ss_family, get_in_addr((struct sockaddr*)&theiraddr),s, sizeof(s));
		printf("server connections from: %s\n", s);
		while((strlen=read(new_fd,buf,MAXDATASIZE)) > 0) {
			printf("received: %s\n", buf); 
			if((strncmp(buf,"quit",4) == 0 ) || (strncmp(buf,"exit",4 == 0)))  
				{ close(new_fd); pthread_exit(NULL); break; }
			 write(new_fd,buf,MAXDATASIZE); 	
			if(strlen<0) { close(new_fd); printf("read error"); }
		}		
		close(new_fd);
	}
}


	



int main(void) {
	int i;
	int sockfd; //listen on sockfd, new connection on new_fd
	struct addrinfo hints, *servinfo;
	int rv;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags	 = AI_PASSIVE;
	int yes = 1;

	if((rv = getaddrinfo(NULL,"3396",&hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	if((sockfd = socket(servinfo->ai_family,servinfo->ai_socktype,servinfo->ai_protocol)) == -1) {
perror("server: socket");
	}
	if(setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
		perror("setsockopt");
		exit(1);
	}
	if(bind(sockfd,servinfo->ai_addr,servinfo->ai_addrlen) == -1)  {
		close(sockfd);
		perror("bind");
	}
	freeaddrinfo(servinfo);

	if(listen(sockfd, 10) == -1) {
		perror("listen");
	}
	pthread_t thread[10];
	for(i = 0; i<10; i++) {
		pthread_create(&thread[i], NULL, clientHandler, (void*)&sockfd);
		}
	

	for(i = 0; i<10; i++) {
		pthread_join(thread[i],NULL);
		}
	return 0;

}



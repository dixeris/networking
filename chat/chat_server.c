#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "socketheader.h"

#define MAXDATASIZE 100

void sigchld_handler(int s) {
	int saved_errno = errno;
	while(waitpid(-1,NULL,WNOHANG) >0);
	errno = saved_errno;
}

void *get_in_addr(struct sockaddr *sa) {
	if(sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
}


int main(void) {
	int sockfd, new_fd; //listen on sockfd, new connection on new_fd
	struct addrinfo hints, *servinfo;
	struct sockaddr_storage theiraddr;
	int rv;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags	 = AI_PASSIVE;
	char s[INET6_ADDRSTRLEN];
	struct sigaction sa;
	int yes = 1;
	socklen_t sin_size;
	char buf[MAXDATASIZE];

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
	while(1) {
	sin_size = sizeof(theiraddr);
	new_fd=accept(sockfd, (struct sockaddr *)&theiraddr, &sin_size);
	if(new_fd==-1) {
		perror("accept");
		continue;
		}
	
	
	sa.sa_handler = sigchld_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if(sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}
	inet_ntop(theiraddr.ss_family, get_in_addr((struct sockaddr*)&theiraddr),s, sizeof(s));
	printf("server connections from: %s\n", s);
	int strlen;
	while((strlen=read(new_fd,buf,MAXDATASIZE)) > 0)  write(new_fd,buf,MAXDATASIZE); 
	if(strlen<0) { close(new_fd); printf("read error"); }
       	

		}
		}


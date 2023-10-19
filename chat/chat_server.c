#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "socketheader.h"
#include <sys/poll.h>

//get ip address in sin_addr format 
void *get_in_addr(struct sockaddr *sa) {
	if(sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
}


//for getting listener file descriptor 
void getlistener(struct addrinfo* hints, struct addrinfo* serverinfo, int* listener) {
	int yes = 1;
	int rv;
	if((rv=getaddrinfo(NULL,"3396",hints,&serverinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
	}
	if(((*listener)=socket(serverinfo->ai_family,serverinfo->ai_socktype,serverinfo->ai_protocol)) == -1) { perror ("listener"); }
	setsockopt((*listener),SOL_SOCKET,SO_REUSEADDR, &yes , sizeof(int));
	if((bind((*listener),serverinfo->ai_addr,serverinfo->ai_addrlen)) == -1) { perror ("bind"); }
	freeaddrinfo(serverinfo);
	if((listen((*listener),10)) == -1) { perror("listen"); }
}

//for pushing a element to pfds 
void push_to_pfds(struct pollfd* pfds[], int conn, int* fd_count,int* fd_size) {
	if(*fd_count == *fd_size) {
		*fd_size *= 2;
		*pfds = realloc(*pfds, sizeof((**pfds)) * (*fd_size));
	}//end for if fd_count == fd_size 

	(*pfds)[*fd_count].fd = conn;
	(*pfds)[*fd_count].events = POLLIN;

	(*fd_count)++;
}	

//for removing a element from pfds 
void remove_from_pfds(struct pollfd* pfds[], int i, int* fd_count, int* fd_size) {
	int num = *fd_size - 1;
	for ( int j = i -1; j < num; j++) {
		(*pfds)[j] = (*pfds)[j+1];	
	}
	(*fd_count)--;
}

int main() {
	int listener, conn;	
	int rv;
	struct addrinfo hints, *serverinfo;
	struct sockaddr_storage remoteaddr;

	char remoteIP[INET_ADDRSTRLEN];
	char buffer[256];	

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getlistener(&hints,serverinfo,&listener);
			
	
	unsigned int fd_count = 0; //current count  of file descriptor 
	int fd_size = 5; //max size of file descriptor 

	struct pollfd* pfds = malloc((sizeof *pfds) * fd_size);
	pfds[0].fd = listener;
	pfds[0].events = POLLIN;
	fd_count = 1;

	for(;;) {
	int poll_count = poll(pfds,fd_count, -1);

	if(poll_count == -1) {
		perror("poll");
		break;
	}
		
	if(poll_count > 0) {
			for(int i = 0; i < fd_count; i++) {

				if(pfds[i].revents & POLLIN) {

					if(pfds[i].fd == listener) {
					//if matched fd is listner

						socklen_t addrlen = sizeof(remoteaddr);

						conn = accept(listener,(struct sockaddr*)&remoteaddr, &addrlen);
							
						push_to_pfds(&pfds,conn,&fd_count,&fd_size);						

						inet_ntop(remoteaddr.ss_family,get_in_addr((struct sockaddr*)&remoteaddr),remoteIP,INET_ADDRSTRLEN);

						printf("server connected from client IP: %s\n", remoteIP);						
						

					}//end if pfds == listener?

					else { //if machted fd is for data 
						int strlen = read(pfds[i].fd, buffer, sizeof(buffer));
					
						if((strncmp(buffer,"quit",4) == 0) || (strncmp(buffer,"exit",4) == 0)) {
							printf("client%d sent exit...\n", i);

							close(pfds[i].fd);

							remove_from_pfds(&pfds,i,&fd_count,&fd_size);


						}//end if strncmp quit or exit 

						else {	
							printf("reading and sending..\n");
							for (int j = 0; j <  fd_count; j++) { 
								if((pfds[j].fd == pfds[i].fd) || (pfds[j].fd == listener)) {
									; //do nothing for sender file descriptor and listener file descriptor 
								} 
							else {
								write(pfds[j].fd,buffer,strlen);
							} //end else statement for writing to all fds
						} //end for statement for broadcasting 

						 }//end else statement for data sending 

					}//end else in case of data available
				}//end if revents & POLLIN 

			} //end for statement 
				
				
	}//end while 
}
	free(pfds);
	return 0;
}//end main function 

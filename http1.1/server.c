#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "socketheader.h"
#include <time.h>

#define PORT "8080"


/*const char* cutBefore(char* argv) { //cut Before character ":" //These lines may not be needed. Only GetValue function is required?
	char* ptr; 
	int c = ':';
	ptr = strchr(argv,c);
	int mystrlen = (strlen(argv) - strlen(ptr));
	char* my_char = malloc(mystrlen);
	memcpy(my_char,argv,mystrlen);	
	return *my_char;
}

BOOLEAN GetFlag(int argc, char* argv[], char* name) {

	const size_t nameLen = strlen(name);	
	for(int i = 0; i < argc; i++) {
		char* new_char = cutBefore(argv[i]);
		strncmp((new_char+1,name,nameLen) == 0) { 
		free(new_char);
		return TRUE;
		}
	}
	return FALSE;
}*/

const char* GetValue(int argc, char* argv[], char* name) { //Get value after : word 
	const size_t nameLen = strlen(name);
	for (int i=0; i < argc; i++) {
		if(strncmp(argv[i]+1,name,nameLen) == 0 
		&& strlen(argv[i]) > 1+nameLen+1 
		&& *(argv[i] + 1 + nameLen) == ':') {
			return argv[i] + 1  + nameLen + 1;
		}
		else { 
			return "NULL";
		}
	

	}
}

const int GetListener(void) { //function for getting listener file descriptor 
	struct addrinfo hints, *serverinfo;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	int listener, rv;
	int yes = 1;

	if((rv=getaddrinfo(NULL,PORT,&hints,&serverinfo)) != 0) { 
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
	}

	if((listener=socket(serverinfo->ai_family,serverinfo->ai_socktype,serverinfo->ai_protocol)) == -1) {
		perror("listen");
	}

	setsockopt(listener,SOL_SOCKET,SO_REUSEADDR, &yes, sizeof(int));

	if((bind(listener, serverinfo->ai_addr, serverinfo->ai_addrlen) == -1)) { perror ("bind"); }
	freeaddrinfo(serverinfo);
	if((listen(listener,10)) == -1) { perror ("listen"); }
	return listener;
}

char* setHeader(char* realpath, char* Header) {	

	char* indexpath = "/index.html";	
	strcat(realpath, indexpath);
	printf("realpath after = %s\n", realpath);
	FILE* htmldata;
	if((htmldata  = fopen(realpath, "r")) == 0) {
		perror("fopen");
		exit(1);
	}
	free(realpath);


	char line[100];
	int current_size = 8000;
	char* responsedata = malloc(current_size); //if size of the responsedata is too  small for the file.. seg fault error will occur.
	int inserted_size = 0;
	while(fgets(line,100,htmldata) != 0) {
		if (inserted_size == current_size ) { responsedata = realloc(responsedata, (2 * current_size)); current_size *= 2; };
		printf("%s",line);		
		strcat(responsedata,line);	
		inserted_size += 100;
	}	

		printf("size of responsedata is %d\n", strlen(responsedata));
		Header = realloc(Header,current_size+25);
		strcat(Header,responsedata);
		return Header;
//		free(responsedata); //invalid pointer error a
//		fclose(htmldata); //corruption error 

}

void handle_client(char* realpath,int connection) {	
	char* buffer = malloc(100);
	char* httpOKHeader  = malloc(8000);
	strcpy(httpOKHeader,"HTTP/1.1 200 OK\r\n\n");
	if((read(connection,buffer,100)) == -1) { 
		perror("read");
	}
	
	else {	
		printf("received:\n%s\n", buffer);
		httpOKHeader = setHeader(realpath, httpOKHeader);
		write(connection,httpOKHeader,strlen(httpOKHeader)); 
		free(httpOKHeader);
		}

		close(connection);
		free(buffer);

}
	

void RunServer(int argc, char* argv[]) { //Running server
	int listener, connection;
	const char* rootpath;

	listener = GetListener();


	if((strncmp((rootpath=GetValue(argc, argv,"default")),"NULL",4)) == 0) { //if root path is not specified, default root path will set.
		rootpath = "/var/www/html";	
		printf("rootpath = %s\n", rootpath);
	}
	
	char* realpath = malloc(255);
	strcpy(realpath,rootpath);
	printf("realpath before = %s\n", realpath);

	for(;;) {	
		if((connection = accept(listener,NULL,NULL)) == -1 )  {
			perror("accept");	
		}
		handle_client(realpath,connection);
	}		

return; 		
}


int main(int argc, char* argv[]) {
	RunServer(argc, argv);

	return 0;
}

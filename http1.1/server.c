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

const char* GetValue(int argc, char* argv[], char* name) { //get value after : word 
	const size_t namelen = strlen(name);
	for (int i=0; i < argc; i++) {
		if(strncmp(argv[i]+1,name,namelen) == 0) {
			return argv[i] + 1  + namelen + 1;
		}
	}
	return "NULL";
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

char* setGETHeader(char* dirpath, char* Header, char* path) {	

	char* dirpath2 = malloc(255);
	strcpy(dirpath2,dirpath);	//setting directory path specified by args 

	char lastpath = path[(strlen(path)-1)];
	if(lastpath == '/') {
	
		strcat(path,"/index.html");
	} //if last file is not specified in request, append the default index file path 

	strcat(dirpath2, path);
	printf("dirpath after = %s\n", dirpath2);
	FILE* htmldata;
	if((htmldata  = fopen(dirpath2, "r")) == 0) {
		strcpy(Header, "HTTP/1.1 404 Not Found\r\nContent-Type:text/html\r\n\n404 File Not Found\n");		
		return(Header);
		perror("fopen");		
		exit(1);
	}
	free(dirpath2);


	char line[100];
	int current_size = 8000;
	char* responsedata = malloc(current_size); //if size of the responsedata is too  small for the file.. seg fault error will occur.
	strcpy(responsedata,"\0");  //solve the below commented problem 
	int inserted_size = 0;

	while(fgets(line,100,htmldata) != 0) {
		printf("%s",line);

		if (inserted_size == current_size ) { 
			responsedata = realloc(responsedata, (2 * current_size));  //realloc() invalid next size at second try 
			current_size *= 2; 
		}

		strcat(responsedata,line);	
		inserted_size += 100;
	}	

		printf("size of responsedata is %d\n", strlen(responsedata));
		Header = realloc(Header,current_size+25); 
		strcat(Header,responsedata);
		free(responsedata);
		responsedata = NULL;
		return Header;

}

void handle_client(char* dirpath,int connection) {	//setting the response message, reading the request line, 
	char* buffer = malloc(100);
	char* httpHeader  = malloc(8000);

	if((read(connection,buffer,100)) == -1) { 
		perror("read");
		exit(1);
	}
	printf("received:\n%s\n", buffer); //reading the request line 

	char* method = strtok(buffer, " "); 
	char* path = strtok(NULL, " ");
	if((strcmp(method,"GET")) == 0)  { //if requested method is "GET", 

		strcpy(httpHeader,"HTTP/1.1 200 OK\nContent-Type:text/html; charset=utf-8\r\n\n"); //setting the response Header

		httpHeader = setGETHeader(dirpath, httpHeader, path); //setting the response body 
		printf("httpHeader = %s\n", httpHeader);
		write(connection,httpHeader,strlen(httpHeader));  //sending to a client 
	}


	/*else if (strcmp(method = strtok(buffer," ")),"POST") == 0} {

			strcpy(httpOKHeader,"HTTP/1.1 200 OK\n\Content-Type:text/html; charset=utf-8\r\n\n"); //setting the response Header

	}*/		/*Other methods processing codes will be created*/	

	
	free(httpHeader);
	httpHeader = NULL; 

	close(connection);
	free(buffer);

}
	

void RunServer(int argc, char* argv[]) { //creating variables indicating the index file location specified by args, and running accept loop with handle_client function 
	int listener, connection;
	const char* rootpath = GetValue(argc, argv, "default");
	printf("rootpath = %s\n", rootpath);
	listener = GetListener();

	if((strncmp(rootpath,"NULL",4)) == 0) { //if root path is not specified, default root path will set.
		rootpath = "/var/www/html";	
		printf("rootpath = %s\n", rootpath);
	}  //setting the file location 
	
	char* dirpath = malloc(255); 
	strcpy(dirpath,rootpath);
	printf("dirpath before = %s\n", dirpath); 

	for(;;) {	
		if((connection = accept(listener,NULL,NULL)) == -1 )  {
			perror("accept");		
			exit(1);
		}
		handle_client(dirpath,connection);
	}		

return; 		
}


int main(int argc, char* argv[]) {
	RunServer(argc, argv);

	return 0;
}

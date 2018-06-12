
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <strings.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include  <signal.h>


#define BUFF_SIZE   12000
#define CLNT_SIZE  1024



struct sockaddr_in clntAddr;
FILE *file_pt;
int num_task;
int sdclient;
int serv_sock;


void sendandreceive (int sd);
void sendFunction( int sd, char *buff3 );
char *readFile (FILE *file_pt);
int creating_socket( char *port);
void control_c_int(int);
/*
*struct contains
*taskID eg O or E
*text the job text
*/
struct taskInfo{
    char taskID;
    unsigned char len;
    char text[256];
};
/*
*Open file
*create socket
*accept connection from the first client and block the rest.
*/

int main(int argc, char *argv[]){

		signal(SIGINT, control_c_int);

		// port number from the user
		if (argc != 3){
			fprintf(stderr,"Usage: filename  port number\n");
			exit(1);
		}


		if ((file_pt = fopen(argv[1], "r")) == NULL){
				printf("Error! opening file");
				exit(1);
		}


		serv_sock = creating_socket( argv[2]);

		if (serv_sock==-1){
			perror("socket failed!");
			exit(EXIT_FAILURE);
		}

		struct sockaddr_in client_vAddr;
		memset(&client_vAddr, 0, sizeof(client_vAddr));

		socklen_t clientlen = sizeof(client_vAddr);

		printf("connecting .... \n");

		for (;;){

			if((sdclient = accept(serv_sock, (struct sockaddr *) &client_vAddr, &clientlen)) < 0){
				perror("accept failed!");
				exit(1);
			}

			printf("connection , socket : %d , IP  : %s , port number : %d \n" , sdclient , inet_ntoa(client_vAddr.sin_addr) , ntohs(client_vAddr.sin_port));

			sendandreceive(sdclient);


		 }

	return 0;
}
/*
*create sockets
*bind and listen to in coming connections.
*use setsock for port reuse.
*/

int creating_socket(char *port){
		int activate = 1;
			// creat socket for connection
		int serv_sock1;   // socket descriptor for server
		if ((serv_sock1 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
			perror("socket failed!");
			return -1;
		}

		int s_sock1 = setsockopt(serv_sock1, IPPROTO_TCP, TCP_NODELAY, &activate, sizeof(int));
    	if(s_sock1 < 0){
	     perror("socket failed!");
	     exit(1);
    	}

    	int s_sock2 = setsockopt(serv_sock1, SOL_SOCKET, SO_REUSEADDR,&activate, sizeof(int));
   		if(s_sock2 < 0){
	     perror("socket failed!");
	     exit(1);
    	}



		struct sockaddr_in serv_addr;
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = INADDR_ANY;
		int p_number = atoi(port);
		serv_addr.sin_port = htons(p_number);

		char hostname[CLNT_SIZE];
		hostname[CLNT_SIZE-1] = '\0';
		gethostname(hostname, CLNT_SIZE-1);
		printf("Host name :  %s\n", hostname);

		// binding to address
	 	if ((bind(serv_sock1, (struct sockaddr *) &serv_addr, sizeof(serv_addr))) < 0){
			perror("binding fail!");
			return -1;
		}
		printf("Listener on port %d \n", p_number);
	 	// socket ready
	 	if((listen(serv_sock1,SOMAXCONN)) < 0){
			perror("listen failure!");
			return -1;
	 	}

	 return serv_sock1;

}
/*
*send and recieve messages
*I've used additional char 'T', to help in communication between server and client when terminating
*if the buf2 recieved from the client is "T", then, close socket and quit
*if recieved byte "G" then read one message from the file.
*/

void sendandreceive (int sd) {
	char buff[BUFF_SIZE];
	char buf2;
	for(;;) {

		bzero(buff,BUFF_SIZE);


		int rc = recv(sd,buff,sizeof(buff),0);
		if (rc < 0) {
			perror("error receiving");
		}
		if (rc == 0) {
			perror("peer has closed its half side of the connection");
		}
		printf("the command is : %s\n",buff);
		buf2 = buff[0];
		if (buf2=='T'){
			printf( "Client terminating normally \n");
			close(sd);
			exit(EXIT_SUCCESS);

		}
		if (buf2=='G' ){
			char *p= readFile(file_pt);
			if(p[0]=='Q'){
				//printf(" message is %c\n", p[0] );

			} else {
				//printf("message is   %s\n", p+2);
			}

			sendFunction(sd, p);
			//bzero(p, sizeof(p));
			free(p);
		}

	}
}
/*
* checks the if the send function returns an error and send buffer to the client.
*/
void sendFunction( int sd, char *buffs ){
		char sendBuf[BUFF_SIZE];

		strcpy(sendBuf, buffs);

		int sn = send(sd,sendBuf,sizeof(sendBuf),0);
		if (sn < 0){
			perror("error sending to client");
		}
		bzero(sendBuf,BUFF_SIZE);


}
/*
*For everytime the readfile is called the fread reads one task.
*if all the tasks have been read and sent, return 'Q'
*/
char *readFile (FILE *file_pt){


	struct taskInfo rec1;
	int j =0;
    char *tempbuff =  malloc(300);
    bzero(tempbuff,300);


	if(feof(file_pt)){
		char endfile [2];
		bzero(endfile,2);
    strcpy(endfile,"Q");
		//bzero(endfile,2);
		strcpy(tempbuff, endfile);

	} else {


  	bzero(rec1.text, 256);
    fread(&rec1.taskID, sizeof(rec1.taskID), 1, file_pt);
    fread(&rec1.len, sizeof(rec1.len), 1, file_pt);
    j=rec1.len;
    fread(&rec1.text, j, 1, file_pt);

	   strcat(tempbuff, &rec1.taskID );

    }



  if(tempbuff[0]!='E' && tempbuff[0]!='O' ){
    char endfile [2];
		bzero(endfile,2);
    strcpy(endfile,"Q");
		//bzero(endfile,2);
    printf("corrupt file\n" );
		strcpy(tempbuff, endfile);

   }
   if(tempbuff[0]!='Q'  ){
     int len  = strlen(tempbuff);
     int strlen = tempbuff[1]-'a'+97;
     if(len-2!=strlen){
       char endfile [2];
   		bzero(endfile,2);
       strcpy(endfile,"Q");
   		//bzero(endfile,2);
      printf("CORRUPT FILE:  incorrect length\n");
   		strcpy(tempbuff, endfile);
     }
   }


    return tempbuff;

}
/*
*when server get ctl-c signal, then the server exits.
*/

void  control_c_int(int signalInt) {
    signal(signalInt, SIG_IGN);
    printf("\n server quiting \n");
    exit(0);

}

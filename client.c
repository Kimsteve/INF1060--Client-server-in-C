#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include  <signal.h>
#define BUFF_SIZE 12000

void sendandreceive(char *str, int a);
void displaymenu();
void MenuOptions();
void  client_connect(char *hostname, char *port);
void printbuff(char *buff);
void  control_c_int(int);
void children_n_pipe(char *hostname, char *port);
void sendtochildren(char *buff);
void terminateProcess();

char buff[BUFF_SIZE];
int sd;
int fd[2]; //file descriptors child one.
int fd2[2]; //file descriptors child two.
int status = 0;
pid_t child_one;
pid_t child_two;


int main( int argc, char *argv[]){
    signal(SIGINT, control_c_int);

    if (argc != 3){
        fprintf(stderr,"client usage %s hostname port number\n", argv[0]);
        exit(0);
    }


    children_n_pipe( argv[1], argv[2]);

  return 0;
}
/* Initialize pipes, fork the children
* child reads jobs sent by the parent.
*/
void children_n_pipe(char *hostname, char *port){
  //int i;
  //int childn = 2;
  int pipret, pipret2;
  pid_t my1, my2;

  //pipe(&fd[2]); //initializing pipes.

  pipret = pipe(fd);
  pipret2 = pipe(fd2);

  child_one = fork();


  if(pipret==-1){
    perror("pipe");
    exit(1);
  }
  if(pipret2==-1){
    perror("pipe");
    exit(1);
  }


  if (child_one == -1) {
        printf("Failure\n");
        exit(EXIT_FAILURE);
  }

  if (child_one == 0) {
      my1 = getpid();

      printf("child 1   %d\n", my1);
      char childbuf [BUFF_SIZE] = {0};
      while(1){

        wait(NULL);
        setbuf(stdout, NULL);
        read(fd[0], childbuf, sizeof(childbuf));
        fprintf(stdout, "child 1 recieved message: %s\n", childbuf+2);
        printf("---------------------------------\n");
        bzero(childbuf,BUFF_SIZE);

        }
      } else {

        child_two = fork();

        if (child_two == 0) {
          my2 = getpid();
          printf("child 2   %d\n", my2 );
          char childbuf [BUFF_SIZE] = {0};
          while(1) {
            wait(NULL);
            read(fd2[0], childbuf, sizeof(childbuf));
            fprintf(stderr, "child 2 recieved message: %s\n", childbuf+2);
            printf("---------------------------------\n");
            bzero(childbuf,BUFF_SIZE);
          }

        } else {

          client_connect( hostname, port);

        }

    }
}
/* create socket
* Connect client to the server
*display the menu
*/

void  client_connect(char *hostname, char *port) {
  // correct arguments should be given from user
  // machine name and port number
  int activate = 1;
  struct sockaddr_in servAddr;
  struct hostent *server;


  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
      perror("socket failed");
      exit(1);
  }

  if(setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, &activate, sizeof(int))< 0){
       perror("socket failed");
       exit(1);
    }

  if ((server = gethostbyname(hostname)) == NULL){
      fprintf(stderr,"error, incorrect hostname \n");
      exit(0);
  }

  int port_number = atoi(port);
  // creating a socket
  bzero((char *) &servAddr, sizeof(servAddr));
  servAddr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *)&servAddr.sin_addr.s_addr, server->h_length);
  servAddr.sin_port = htons(port_number);


  if (connect(sd,(struct sockaddr *) &servAddr, sizeof(servAddr)) < 0)
  {
      perror("error connecting");
      exit(1);
  }

   displaymenu();
}
/*
* recieves @ input_option and checks the content
* if it contains 'T', send the message to server, kill child process and closed socket and exit
* if it is 'G', loop through @int a, and send and recieve jobs.
* if recieved task is 'Q', send 'T'- termination message to server, kill child process and closed socket and exit
* if it it 'A', Get all the jobs or the remaining jobs.
*loop through @int a, send 'G' to request a taks
* and write recieved messages to child processes.
*/

void sendandreceive (char* input_option, int a) {

    bzero(buff,BUFF_SIZE);
    fgets(buff,BUFF_SIZE,stdin);

    strcpy(buff,input_option);
    char buf2 = buff[0];
    if (buf2=='T'){
      int sen = send(sd,buff,sizeof(buff),0);
      if (sen < 0) perror("error sending");
      bzero(buff,BUFF_SIZE);
      close(sd);
      kill(child_one, SIGKILL);
      kill(child_two, SIGKILL);
      exit(EXIT_SUCCESS);
    }


    if (buf2=='G'){

        int r = 0;
        char tbuff[BUFF_SIZE];
        bzero(tbuff,BUFF_SIZE);
        strcpy(tbuff,buff);

        while(r<a){
          strcpy(buff,tbuff);
          int sen = send(sd,buff,sizeof(buff),0);
          if (sen < 0) perror("error sending");
          int rec = recv(sd,buff,sizeof(buff),0);
          if (rec < 0) perror("error receiving from socket");


          if(buff[0]=='Q'){
            char exitmessage [2]= "T";
            int sen = send(sd,exitmessage,sizeof(exitmessage),0);
            if (sen < 0) perror("error sending");
            printf(" Client exiting \n");
            kill(child_one, SIGKILL);
            kill(child_two, SIGKILL);
            exit(0);
          }
          //corruptFileTest(buff);
          sendtochildren(buff);
          r++;
          bzero(buff,BUFF_SIZE);
        }
        bzero(tbuff,BUFF_SIZE);

    } else if (buf2=='A'){
      //char exitmessage [2]= "T";

      char temp_buff[BUFF_SIZE];
      bzero(temp_buff,BUFF_SIZE);
      strcpy(temp_buff,"G");
      char hold_buff[BUFF_SIZE];
      bzero(hold_buff,BUFF_SIZE);
      strcpy(hold_buff,temp_buff);



      int sen = send(sd,temp_buff,sizeof(temp_buff),0);
      if (sen < 0) perror("error on sending to server");
      int rec = recv(sd,temp_buff,sizeof(temp_buff),0);
      if (rec < 0) perror("error receiving from server");

      sendtochildren(temp_buff);


      while(temp_buff[0]!='Q'){
        //strcpy(buff,tbuff2);
        strcpy(temp_buff, hold_buff);
        int sen = send(sd,temp_buff,sizeof(temp_buff),0);
        if (sen < 0) perror("error on sending to server");
        int rec = recv(sd,temp_buff,sizeof(temp_buff),0);
        if (rec < 0) perror("error receiving from server");

        sendtochildren(temp_buff);
      }

      if(temp_buff[0]=='Q'){
        char exitmessage [2]= "T";
        int sen = send(sd,exitmessage,sizeof(exitmessage),0);
        if (sen < 0) perror("error sending");
        printf("No more tasks in the file. Client exiting \n");
        kill(child_one, SIGKILL);
        kill(child_two, SIGKILL);
        close(sd);
        exit(EXIT_SUCCESS);

      }
      bzero(temp_buff,BUFF_SIZE);
      bzero(hold_buff,BUFF_SIZE);
    }

    displaymenu();

}
/*
*Terminates the child process and close the socket
*/
void terminateProcess(){
  kill(child_one, SIGKILL);
  kill(child_two, SIGKILL);
  close(sd);
}
/*
*Display the menu
*/
void displaymenu() {
      printf(" [1] Get a job from the server \n");
      printf(" [2] Get X Number of jobs from the server\n");
      printf(" [3] Get all jobs from the server\n");
      printf(" [4] Quit\n");
      printf(" Please enter your Choice:\n");
      MenuOptions();
}
/*
* handle the choice made by the user.
* If one message is requested send 'G' and int to show number of messages
* If option 2, ask the user the number of jobs
* send 'A', for all messages on option 3
*/
void MenuOptions(){
  int key;
    for(;;){
      scanf("%d",&key);

          switch ( key ) {

              case 1:
                  printf( "Getting job from the server: \n" );
                  sendandreceive("G", 1);

                  break;

              case 2:
                  printf( "How many jobs do you want from the server? \n" );
                  int k;
                  scanf( "%d", &k);
                  printf("chosen %d\n", k);
                  sendandreceive("G", k);

                  break;

              case 3:
                  printf( "Getting all jobs from the server \n" );
                  sendandreceive("A", 1);
                  break;

              case 4:
                  printf("program will end  \n" );
                  sendandreceive("T", 1);
                  break;

              default:
                break;
          }


      }

}
/*
*send different tasks depending on the first char to different child.
* tasks starting with E are sent to child 2
* tasks starting with O are sent to child 1
*/
void sendtochildren(char *buff){
  if(buff[0]=='O'){
    write(fd[1], buff, strlen(buff));
  }
  if(buff[0]=='E'){
    write(fd2[1], buff, strlen(buff));
  }
}


void printbuff(char *buff){
  printf("--------------------------------\n" );
  fprintf(stdout, "from server %s\n", buff+2);
  printf("--------------------------------\n" );
}
/*if ctl-c is detected then call sendandrecieve function
*
*/

void  control_c_int(int signalInt){
     char  yn;

     signal(signalInt, SIG_IGN);

     printf("Do you want to quit client program ? [y] ");
     yn = getchar();
     if (yn == 'y' || yn == 'Y'){
      sendandreceive("T",1);

     } else {
        signal(SIGINT, control_c_int);

    }
    getchar();
}

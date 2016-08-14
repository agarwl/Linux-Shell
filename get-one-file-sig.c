#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h> 

#define min(a, b) ((a < b) ? a : b)
#define NUM_FILES 10000
#define BUF_SIZE 30

int numbytes_read;
struct sigaction old_action;

void sigint_handler(int sig_no)
{
    fprintf(stderr," Received SIGINT; downloaded %d bytes so far.\n",numbytes_read);
    sigaction(SIGINT, &old_action, NULL);
}

int readMyFile(int ,char*); //read file and discard
void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{

    // signal handler
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = &sigint_handler;
    sigaction(SIGINT, &action, &old_action);

    struct hostent *server;
    int portno,runtime;
    char *mode, *file;
    int sockfd;                   // socket file descriptor
    struct sockaddr_in serv_addr; //server address info
    int n;                        

    if (argc < 4) {
       fprintf(stderr,"usage %s filename hostname port mode\n", argv[0]);   //error if less arguments provided
       exit(0);
    }

    portno = atoi(argv[3]);             //set portno as second input argument
    server = gethostbyname(argv[2]);    //set host as first input argument
    file = argv[1];
    mode = argv[4];                     //set file download mode as sixth input argument

    if (server == NULL) {       //check if host exists
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    bzero((char *) &serv_addr, sizeof (serv_addr));

    serv_addr.sin_family = AF_INET;     //server host byte order
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length); //server address   
    serv_addr.sin_port = htons(portno); //server port network byte order   

    sockfd = socket(AF_INET, SOCK_STREAM, 0); //create socket
    if (sockfd < 0)                           //check if successfully created
        error("ERROR opening socket");

    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) //connect to socket and check
        error("ERROR connecting");

   
    char filereq[BUF_SIZE];
    bzero(filereq,BUF_SIZE); 
    strcpy(filereq, "get ");
    strcat(filereq,file);

    n = write(sockfd,filereq,strlen(filereq)); //request server for files
    if (n < 0) error("ERROR writing to socket");

    readMyFile(sockfd,mode); //read the file 1024 bytes at a time and discard them
    close(sockfd);    //close socket

    return 0;
}

int readMyFile(int sock,char* mode) 
{
    bool toPrint = false;
    if(strcmp(mode,"display") == 0)
        toPrint = true;
    char buffer[1024];  //read 1024 bytes at a time
    size_t bufflen = sizeof(buffer); //size of buffer
    int bytes_read;
    numbytes_read = 0;
    while(( bytes_read = read(sock, buffer, bufflen) ) > 0)
    {
        if (strcmp(buffer,"File requested does not exist\n") == 0)
        {
            fprintf(stderr,"%s",buffer);
            break;
        }
        numbytes_read += bytes_read;
        if(toPrint){
            // printf("%d bytes read\n",bytes_read);
            printf("%.*s", bytes_read, buffer);
            // printf("%s",buffer );
        }
    }; 

    if(bytes_read == 0)
        return 1;
    else
        return 0;
}
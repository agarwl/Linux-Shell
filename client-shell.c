#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h> 
#include <stdbool.h>
#include <pthread.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64


// Helper function to tokenize the user input command
void getfl(char **tokens);
void getsq(char **tokens);
void getpl(char **tokens);
void *connection(void* threadid);

char **tokenize(char *line)
{
  char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
  char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
  int i, tokenIndex = 0, tokenNo = 0;

  for(i =0; i < strlen(line); i++){

    char readChar = line[i];

    if (readChar == ' ' || readChar == '\n' || readChar == '\t'){
      token[tokenIndex] = '\0';
      if (tokenIndex != 0){
        tokens[tokenNo] = (char*)malloc(MAX_TOKEN_SIZE*sizeof(char));
        strcpy(tokens[tokenNo++], token);
        tokenIndex = 0; 
      }
    } else {
      token[tokenIndex++] = readChar;
    }
  }

  free(token);
  tokens[tokenNo] = NULL ;
  return tokens;
}

void error(char *msg)
{
    perror(msg);
    exit(0);
}

int child_pid;
int ppgid;
struct sigaction old_action;
char portno[MAX_TOKEN_SIZE],host[MAX_TOKEN_SIZE];
 
// char s[]="Type 'exit' to terminate\n";
void freeMemory(char**);

void sigint_handler(int sig_no)
{   
    printf("\n");
    if(child_pid != 0){
    //   printf("%d in handler\n",child_pid);
      kill(child_pid, SIGINT);
    }
}
char COMMANDS[][8] = {"cd","server","getfl","getsq","getpl","getbg","exit"};
int find_id(char * x)
{
  // id denotes the manually alotted id to the command
  int id = -1,i;
  if(x == NULL)
    return -1;
  for(i=0;i<8;i++)
  {
      if(strcmp(x,COMMANDS[i]) == 0){
          id = i+1;
          break;
      }
  }
  if(id == -1)
    id = 0; // command id for executable binaries

  return id;
}

// timeout value to periodically kill dead children
// after an interval of 5s
int timeout = 2;

// a function to kill zombies periodically
void *kill_zombies()
{
    int w;
    while(1){
        while (( w = waitpid(-1, NULL, WNOHANG) ) > 0)
        {
            if(getpgid(w) != ppgid){
              printf("Background process with pid %d completed\n",w);
            }
            // printf("Killed zombie %d\n", w);
        }
        sleep(timeout);
    }
}

int main(void)
{
  char line[MAX_INPUT_SIZE];
  char **tokens;            
  int i;           
  int pid,myid;
  int command_id;
  bool toBreak = false;
  ppgid = getpgid(getpid());
  printf("Parent's %d\n",ppgid);
  // for(i=0;i<8;i++)
  //   printf("%s\n",COMMANDS[i]);

  struct sigaction action;
  memset(&action, 0, sizeof(action));
  sigemptyset (&action.sa_mask);
  action.sa_flags = 0;
  action.sa_handler = &sigint_handler;
  sigaction(SIGINT, &action, &old_action);

  // create thread for reaping dead child
  pthread_t my_thread;
  if(pthread_create( &my_thread , NULL ,  kill_zombies , NULL) < 0)
      perror("could not create thread");


  while (1) {           

    child_pid = myid = 0;
    printf("Hello>");     
    bzero(line, MAX_INPUT_SIZE);
    fgets(line, MAX_INPUT_SIZE, stdin);
    tokens = tokenize(line);

    command_id = find_id(tokens[0]);
    
    switch(command_id){
      
      case 1:
        if(chdir(tokens[1]) < 0)
          printf("cannot cd %s\n", tokens[1]);
        break;

      case 2:
          strcpy(host,tokens[1]);
          strcpy(portno,tokens[2]);
          break;

      case 7:
        // kill(0,SIGINT);
        kill(0,SIGINT);
        kill(myid,SIGINT);
        toBreak = true;
        break;
      
      default:

        pid = fork();
        
        if(pid == -1){
          printf("Fork Failed\n");
          exit(1);
        }
        else if(pid == 0){

          switch(command_id){

            case 0:
              if( execvp(tokens[0],tokens) == -1)
                perror("Exec failed");
                break;

            case 3:
                getfl(tokens);
                break;

            case 4:
                getsq(tokens);
                break;

            case 5:
                getpl(tokens);
                break;

            case 6:
                myid = getpid();
                setpgid(myid,0);
                printf("Background process with pid %d and pgid %d started\n",getpid(),getpgid(pid));
                getfl(tokens);
                break;

            default:
              break;
          }
          exit(0);
        }
        else{
          // printf("Child's %d\n",getpgid(pid));
          if(command_id !=  6){
            child_pid = pid;
            waitpid(pid, NULL, 0);
          }
          else{
            myid = pid;
            child_pid = 0;
            // printf("Background process with pid %d and pgid %d started\n",pid,getpgid(pid));
          }
        }
    }

    //Freeing the allocated memory 
    for(i=0;tokens[i]!=NULL;i++){
      free(tokens[i]);
    }
    free(tokens);

    if(toBreak)
      break;
  }

  exit(0);
}

void getfl(char **tokens)
{
    int i;
    for (i = 2; i < 5; ++i)
    {
      tokens[i] = (char*)malloc(MAX_TOKEN_SIZE*sizeof(char));
    }
    strcpy(portno,"5000");
    strcpy(host,"localhost");
    strcpy(tokens[0],"./get-one-file-sig");
    strcpy(tokens[2],host);
    strcpy(tokens[3],portno);
    strcpy(tokens[4],"display");
    tokens[5] = NULL;
    for (i = 0; tokens[i] != NULL; ++i)
    {
        printf("%s\t",tokens[i]);
    }
    if( execvp(tokens[0],tokens) == -1)
        perror("Exec failed");
    exit(0);
}


void getsq (char **tokens)
{
    int i;
    char *newtokens[6];

    for (i = 0; i < 5; ++i)
    {
      newtokens[i] = (char*)malloc(MAX_TOKEN_SIZE*sizeof(char));
    }
    strcpy(portno,"5000");
    strcpy(host,"localhost");
    strcpy(newtokens[0],"./get-one-file-sig");
    strcpy(newtokens[2],host);
    printf("%s\n", host);
    strcpy(newtokens[3],portno);
    printf("%s\n", portno);
    strcpy(newtokens[4],"nodisplay");
    newtokens[5] = NULL;
    i = 1;
    while (tokens[i] != NULL)
    {
        strcpy(newtokens[1],tokens[i]);
        int j;
        for (j = 0; newtokens[j] != NULL; ++j)
        {
            printf("%s ",newtokens[j]);
        }
        if( execvp(newtokens[0],newtokens) == -1)
            perror("Exec failed");
        exit(0);
        i++;
    }
    for (i = 0; i < 5; ++i)
      free(newtokens[i]);
    return;
}

void getpl (char **tokens)
{
    pthread_t tid[MAX_NUM_TOKENS];          //array of threads 
    int i;
    for (i=1; tokens[i] != NULL; i++) //create threads by calling function 'connection' and passing thread number as argument
    {   
        if( pthread_create( &tid[i] , NULL ,  connection , (void*) tokens[i]) < 0)      
        {
            error("could not create thread");
        }
    }
    int j;
    for (j = 1; j < i; j++)
       pthread_join(tid[j], NULL);      //join the threads when all have been executed
    return;
}

void *connection(void *threadid)
{
    int i;
    printf("Thread: %s\n", (char*) threadid);
    char *newtokens[6];
    for (i = 0; i < 5; ++i)
    {
      newtokens[i] = (char*)malloc(MAX_TOKEN_SIZE*sizeof(char));
    }
    strcpy(portno,"5000");
    strcpy(host,"localhost");
    newtokens[0] = "./get-one-file-sig";
    newtokens[1] = (char*) threadid;
    newtokens[2] = host;
    newtokens[3] = portno;
    newtokens[4] = "nodisplay";
    newtokens[5] = NULL;
    if( execvp(newtokens[0],newtokens) == -1)
        perror("Exec failed");
    exit(0);
}

// getpl files/foo0.txt files/foo1.txt
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

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64

// Helper function to tokenize the user input command
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
struct sigaction old_action;
char portno[MAX_TOKEN_SIZE],hostname[MAX_TOKEN_SIZE];
 
// char s[]="Type 'exit' to terminate\n";
void freeMemory(char**);

void sigint_handler(int sig_no)
{   
    printf("\n");
    // freeMemory(tokens);
    // sigaction(SIGINT, &old_action, NULL);
    // write(fileno(stdin), s, sizeof s - 1);
    if(child_pid != 0)
      kill(child_pid, SIGINT);
    // exit(0);
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
        while (( w = waitpid(-1, NULL, WNOHANG) ) > 0)//{
                // printf("Killed zombie %d\n", w);
        // }
        sleep(timeout);
    }
}



int main(void)
{
  char line[MAX_INPUT_SIZE];
  char **tokens;            
  int i;           
  int pid;
  int command_id;
  bool toBreak = false;

  for(i=0;i<8;i++)
    printf("%s\n",COMMANDS[i]);

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

    child_pid = 0;
    printf("Hello>");     
    bzero(line, MAX_INPUT_SIZE);
    // gets(line);
    fgets(line, MAX_INPUT_SIZE, stdin);
    tokens = tokenize(line);
    
    if(tokens[0] != NULL){
      if(strcmp(tokens[0],"exit") == 0)
      toBreak = true;
    }

      command_id = find_id(tokens[0]);
      switch(command_id){
        
      case 1:
        if(chdir(tokens[1]) < 0)
          printf("cannot cd %s\n", tokens[1]);
        break;

      case 2:
          strcpy(hostname,tokens[1]);
          strcpy(portno,tokens[2]);
          break;

      case 7:
        kill(0,SIGINT);
        toBreak = true;
      
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
            case 2:


            default:
              break;
          }
          exit(0);
        }
        else{

          if(command_id !=  6){
            child_pid = pid;
            waitpid(pid, NULL, 0);
          }
        }
        break;
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
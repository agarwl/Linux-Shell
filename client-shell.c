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
 
// char s[]="Type 'exit' to terminate\n";
void freeMemory(char**);

void sigint_handler(int sig_no)
{   
    printf("\n");
    // freeMemory(tokens);
    // sigaction(SIGINT, &old_action, NULL);
    // write(fileno(stdin), s, sizeof s - 1);
    if(child_pid != 0)
      kill(child_pid, SIGKILL);
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

    pid = fork();
    if(pid == -1){
      printf("Fork Failed\n");
      exit(1);
    }
    else if(pid == 0){

      command_id = find_id(tokens[0]);
      switch(command_id){

        case 0:
          if( execvp(tokens[0],tokens) == -1)
            perror("Exec failed");
          break;

        case 7:
          kill(0,SIGKILL);
          toBreak = true;
          break;

        default:
          break;

      }

      exit(0);
    }
    else{
      child_pid = pid;
        // printf("in parent\n");
      waitpid(pid, NULL, 0);
    }

    //Freeing the allocated memory 
    for(i=0;tokens[i]!=NULL;i++){
      free(tokens[i]);
    }
    free(tokens);

    if(toBreak)
      break;

    // // Read and run input commands.
    // while(getcmd(buf, sizeof(buf)) >= 0){
    //   if(buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' '){
    //     // Clumsy but will have to do for now.
    //     // Chdir has no effect on the parent if run in the child.
    //     buf[strlen(buf)-1] = 0;  // chop \n
    //     if(chdir(buf+3) < 0)
    //       printf(2, "cannot cd %s\n", buf+3);
    //     continue;
    //   }
    //   if(fork1() == 0)
    //     runcmd(parsecmd(buf));
    //   wait();
  }

  exit(0);
}
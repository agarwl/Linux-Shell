#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h> 

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


void sigint_handler(int sig_no)
{
    printf("CTRL-C pressed\n");
    kill(child_pid, SIGINT);
}

int main(void)
{
  char  line[MAX_INPUT_SIZE];            
  char  **tokens;              
  int pid;

  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_handler = &sigint_handler;
  sigaction(SIGINT, &action, &old_action);



  while (1) {           

    printf("Hello>");     
    bzero(line, MAX_INPUT_SIZE);
    // gets(line);
    fgets(line, MAX_INPUT_SIZE, stdin);
    tokens = tokenize(line);

    pid = fork();
    if(pid == -1){
      printf("Fork Failed\n");
      exit(1);
    }
    else if(pid == 0){
      // printf("in child\n");
      if( execvp(tokens[0],tokens) == -1)
        perror("Exec failed");
      exit(0);
    }
    else{
      child_pid = pid;
        // printf("in parent\n");
      waitpid(pid, NULL, 0);
    }

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
#include  <stdio.h>
#include  <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64

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

int child_pid;
struct sigaction old_action;

void sigint_handler(int sig_no)
{   
    printf("CTRl-C pressed\n");
    if(child_pid != 0)
      kill(child_pid, SIGKILL);
    // exit(0);
}


void  main(void)
{
     char  line[MAX_INPUT_SIZE];            
     char  **tokens;              
     int i;
     bool toBreak = false;

    struct sigaction action;
    // memset(&action, 0, sizeof(action));
    sigemptyset (&action.sa_mask);
    action.sa_flags = 0;
    action.sa_handler = &sigint_handler;
    sigaction(SIGINT, &action, &old_action);

     while (1) {           
       
       child_pid = 0;
       printf("Hello>");     
       bzero(line, MAX_INPUT_SIZE);
       fgets(line,MAX_INPUT_SIZE,stdin);           
       // printf("Got command %s\n", line);
       tokens = tokenize(line);

       if(tokens[0] != NULL){
         if(strcmp(tokens[0],"exit") == 0)
          toBreak = true;
       }

      //  if(strcmp(tokens[0],"exit") == 0){
      //   // kill(0,SIGKILL);
      //   free(tokens[0]);
      //   free(tokens);
      //   break;
      // }
         
       //do whatever you want with the commands, here we just print them

  //      for(i=0;tokens[i]!=NULL;i++){
	 // printf("found token %s\n", tokens[i]);
  //      }
       
       // Freeing the allocated memory	
      for(i=0;tokens[i]!=NULL;i++){
        free(tokens[i]);
      }
      free(tokens);
       if(toBreak)
          break;
     }

     exit(0);

}

                

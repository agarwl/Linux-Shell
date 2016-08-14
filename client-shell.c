#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h> 
// #include <stdbool.h>
#include <pthread.h>
#include <errno.h>
#include <set>
#include <assert.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64


// Helper function to tokenize the user input command
void getfl(char **tokens,bool display=1);
void getsq(char **tokens);
void getpl(char **tokens);
void *connection(void* threadid);
void filltokens(char **newtokens,bool display=0);
void freeMemory(char**);
void myexec(char **tokens);

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

struct sigaction old_action;
char portno[MAX_TOKEN_SIZE],host[MAX_TOKEN_SIZE];
std::set<pid_t> background_pids;


void sigint_handler(int sig_no)
{   
    sleep(0.1);
    printf("\n");
    // printf(" Ctrl-C pressed\n");
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

int parse_getfl(char ** tokens)
{ 
  int i =0;
  int k = 0;
  while(tokens[i] != NULL)
  {
    if(i==2){
      if(strcmp(tokens[i] , ">") == 0)
        k = 1;
      else if(strcmp(tokens[i],"|") == 0)
        k = 2;
      else{
        k = -1;
        perror("Incorrect arguments passed to getfl\n");
      }
    }
    i++;
  }
  return k;
}

void handle_getfl(int k,char **tokens)
{
  int p[2];
  int pid;
  switch(k){

    case 1:
      close(1);
      if(open(tokens[3], O_WRONLY) < 0)
        error("open failed");

    case 0:
      getfl(tokens);
      break;
    
    case 2: 
      if(pipe(p) == -1)
        error("pipe failed");
      
      pid = fork();
      if(pid < 0)
        error("fork");
      else if(pid == 0){
        close(1);
        dup(p[1]);
        close(p[0]);
        close(p[1]);
        getfl(tokens);
      }
      
      pid = fork();
      if(pid < 0)
        error("fork");
      else if(pid == 0){
        close(0);
        dup(p[0]);
        close(p[0]);
        close(p[1]);
        if( execvp(tokens[3],tokens+3) == -1)
          error("Exec failed");
      }
      
      close(p[0]);
      close(p[1]);
      wait(NULL);
      wait(NULL);
      break;
      
    default:
      break;
  }
  exit(0);

}

int checkArguments(int command_id, char**tokens)
{
    if (command_id > 0 && command_id < 7 && tokens[1] == NULL)
    {
        printf("Less arguments provided\n");
        return -1;
    }
    if (((command_id == 6 || command_id == 1) && tokens[2] != NULL) || (command_id == 7 && tokens[1] != NULL))
    {
        printf("More argumnents given\n");
        return -1;
    }
    if (command_id == 2 && (tokens[2] == NULL || tokens[3] != NULL))
    {
        printf("Invalid argumnents\n");
        return -1;
    }
    return 1;
}

// timeout value to periodically kill dead children
// after an interval of 5s
int timeout = 2;

// a function to kill zombies periodically
void *kill_zombies(void *)
{
    pid_t w;
    int status;
    
    while(1){

        while (( w = waitpid(-1, &status, WNOHANG) ) > 0)
        {
            if(background_pids.count(w) != 0){
              background_pids.erase(w);
              if (WIFEXITED(status)) 
                printf("Background process with pid %d completed\n",w);
              else if (WIFSIGNALED(status)) 
                printf("Background process with pid %d exited due to errors!",w);
              printf("Press [ENTER] to continue\n");
            }
        }
        sleep(timeout);
    }
}

int main(void)
{

  memset( host, '\0', sizeof(char)*MAX_TOKEN_SIZE );
  memset( portno, '\0', sizeof(char)*MAX_TOKEN_SIZE );
  char line[MAX_INPUT_SIZE];
  char **tokens;            
  int i;           
  int pid;
  int command_id;
  bool toBreak = false;

  struct sigaction action;
  memset(&action, 0, sizeof(action));
  sigemptyset (&action.sa_mask);
  action.sa_flags = 0;
  action.sa_handler = &sigint_handler;
  sigaction(SIGINT, &action, &old_action);

  // create thread for reaping dead child
  pthread_t my_thread;
  if(pthread_create( &my_thread , NULL ,  kill_zombies , NULL) < 0)
      error("could not create thread");


  while (1) {           

    printf("Hello>");     
    bzero(line, MAX_INPUT_SIZE);
    fgets(line, MAX_INPUT_SIZE, stdin);
    tokens = tokenize(line);

    command_id = find_id(tokens[0]);
    if (checkArguments(command_id,tokens) == -1)
        continue;

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
        kill(0,SIGINT);
        for (std::set<pid_t>::iterator it=background_pids.begin(); 
            it!=background_pids.end(); ++it)
          kill(*it,SIGINT);
        toBreak = true;
        break;
      
      default:

        pid = fork();
        
        if(pid == -1){
          printf("Fork Failed\n");
          exit(1);
        }
        else if(pid == 0){

          sigaction(SIGINT, &old_action, NULL);
          switch(command_id){

            case 0:
              if( execvp(tokens[0],tokens) == -1)
                error("Exec failed");
                break;

            case 3:
                i = parse_getfl(tokens);
                handle_getfl(i,tokens);
                break;

            case 4:
                getsq(tokens);
                break;

            case 5:
                getpl(tokens);
                break;

            case 6:
                assert(setpgid(0,0) == 0);
                // printf("Background process with pid %d started\n",getpid());
                getfl(tokens,0);
                break;

            default:
              break;
          }
          exit(0);
        }
        else{
          if(command_id !=  6){
            waitpid(pid, NULL, 0);
          }
          else{
            if (setpgid(pid, pid) < 0 && errno != EACCES)
              continue;
            background_pids.insert(pid);
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

void getfl(char **tokens,bool display)
{
    char *newtokens[6];
    // strcpy(portno,"5000");
    // strcpy(host,"localhost");
    filltokens(newtokens,display);
    strcpy(newtokens[1],tokens[1]);
    myexec(newtokens);
    for (int i = 0; i < 5; ++i)
      free(newtokens[i]);
}


void getsq (char **tokens)
{
    int pid;
    char *newtokens[6];
 
    // strcpy(portno,"5000");
    // strcpy(host,"localhost");
    filltokens(newtokens);
    int i = 1;
    while (tokens[i] != NULL)
    {
        strcpy(newtokens[1],tokens[i]);
        pid = fork();
        if(pid == -1){
          printf("Fork Failed\n");
          exit(1);
        }
        else if(pid == 0){
            myexec(newtokens);
        }
        else{
          wait(NULL);
          i++;
        }
        
    }
    for (i = 0; i < 5; ++i)
      free(newtokens[i]);
    exit(0);
}


void getpl(char **tokens)
{
    int pid;
    char *newtokens[6];
    // strcpy(portno,"5000");
    // strcpy(host,"localhost");
    filltokens(newtokens);
    int i = 1;
    while (tokens[i] != NULL)
    {
        strcpy(newtokens[1],tokens[i]);
        pid = fork();
        if(pid == -1){
          printf("Fork Failed\n");
          exit(1);
        }
        else if(pid == 0)
        {
            myexec(newtokens);
        }
        i++;
    }
    while(wait(NULL)  > 0);
    for (i = 0; i < 5; ++i)
      free(newtokens[i]);
    exit(0);
}

void filltokens(char **newtokens,bool display)
{
    for (int i = 0; i < 5; ++i)
      newtokens[i] = (char*)malloc(MAX_TOKEN_SIZE*sizeof(char));
    strcpy(newtokens[0],"./get-one-file-sig");
    if(strcmp(host,"") == 0 || strcmp(portno,"") == 0 ){
      printf("Please use server command to specify hostname and portno\n");
      exit(1);
    }
    strcpy(newtokens[2],host);
    strcpy(newtokens[3],portno);
    if(display)
      strcpy(newtokens[4],"display");
    else 
      strcpy(newtokens[4],"nodisplay");
    newtokens[5]=NULL;
} 
void myexec(char **tokens)
{
      if( execvp(tokens[0],tokens) == -1)
          perror("Exec failed");
      exit(0);
}
// getpl files/foo0.txt files/foo1.txt files/foo2.txt files/foo3.txt
// getsq files/foo0.txt files/foo1.txt files/foo2.txt files/foo3.txt
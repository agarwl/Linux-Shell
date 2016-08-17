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
#define PATH_MAX 100

// Helper function to tokenize the user input command
char **tokenize(char *line);
// Helper function to download a file from our server
void getfl(char **tokens,bool display=1);
// Helper function to download a sequence of files from our server
void getsq(char **tokens);
// Helper function to download files parallely
void getpl(char **tokens);
// Function to fill tokens for passing arguments to exec in getfl,getpl,getsq etc
void filltokens(char **newtokens,bool display=0);
// Function to exec which prints an error on failure
void myexec(char **tokens);
// Function to find id associated with a command 
int find_id(char* );
// function to parse/handle getfl command (for piping/ redirection)
int parse_getfl(char ** tokens);
void handle_getfl(int ,char **);

void error(const char*);

// overloading the default signal handler for SIGINT
void sigint_handler(int sig_no);

// error handling for incorrect arguments
int checkArguments(int command_id, char**tokens);

// a function to kill zombies periodically
void *kill_zombies(void *);

// to store the default action of signal handler
struct sigaction old_action;
// to store the portno,hostname
char portno[MAX_TOKEN_SIZE],host[MAX_TOKEN_SIZE];
// to store the pids of running background processes
std::set<pid_t> background_pids;
char COMMANDS[][8] = {"cd","server","getfl","getsq","getpl","getbg","exit"};
// timeout value to periodically kill dead children after an interval of 5s
int timeout = 2;
char file_path[PATH_MAX+1];
pthread_mutex_t lock;

int main(void)
{

  memset(host, '\0', sizeof(char)*MAX_TOKEN_SIZE );
  memset(portno, '\0', sizeof(char)*MAX_TOKEN_SIZE );
  char line[MAX_INPUT_SIZE];
  char **tokens;            
  int i;           
  int pid;
  int command_id;

  // a boolean to idicate whether to break from indefinite while loop or not
  bool toBreak = false;

  char *ptr;
  ptr = realpath("./get-one-file-sig", file_path);


  // overriding the default signal handler
  struct sigaction action;
  memset(&action, 0, sizeof(action));
  sigemptyset (&action.sa_mask);
  action.sa_flags = 0;
  action.sa_handler = &sigint_handler;
  sigaction(SIGINT, &action, &old_action);

  // create thread for reaping dead childs running in background
  pthread_t my_thread;
  if(pthread_create( &my_thread , NULL ,  kill_zombies , NULL) < 0)
      error("could not create thread");


  while (1) {           

    printf("Hello>");  

    // read the command tokens from input   
    bzero(line, MAX_INPUT_SIZE);
    fgets(line, MAX_INPUT_SIZE, stdin);
    tokens = tokenize(line);

    // check if correct arguments passed or not
    command_id = find_id(tokens[0]);
    if (checkArguments(command_id,tokens) == -1)
        continue;

    switch(command_id){
      
      // executed in parent itself
      // on cd command
      case 1:
        if(chdir(tokens[1]) < 0)
          fprintf(stderr,"cannot cd %s\n", tokens[1]);
        break;

      // on server command
      case 2:
          strcpy(host,tokens[1]);
          strcpy(portno,tokens[2]);
          break;

      // on exit command
      case 7:
        // kill all the child with same process group id as the parent's
        kill(0,SIGINT);
        // send SIGINT to all background process
        pthread_mutex_lock(&lock);
        for (std::set<pid_t>::iterator it=background_pids.begin(); 
            it!=background_pids.end(); ++it)
          kill(*it,SIGINT);
        pthread_mutex_unlock(&lock);
        toBreak = true;
        break;
      
      // on other commands
      default:

        // fork a child
        pid = fork();
        
        if(pid == -1){
          error("Fork Failed\n");
        }
        else if(pid == 0){

          // make the signal handler for child as default action for SIGINT 
          sigaction(SIGINT, &old_action, NULL);
          switch(command_id){


            // on simple exec command
            case 0:
              if( execvp(tokens[0],tokens) == -1)
                error("Exec failed");
                break;

            // on getfl 
            case 3:
                i = parse_getfl(tokens);
                handle_getfl(i,tokens);
                break;

            // on getfl
            case 4:
                getsq(tokens);
                break;

            // on getpl
            case 5:
                getpl(tokens);
                break;

            // on getbg
            case 6:
                // check if process group id is set equal to process id 
                assert(setpgid(0,0) == 0);
                getfl(tokens,0);
                break;

            default:
              break;
          }
          exit(0);
        }
        else{
          if(command_id !=  6){
            // blocking wait for child process for foreground commands
            waitpid(pid, NULL, 0);
          }
          else{
            // try to set process group id if not set in child
            if (setpgid(pid, pid) < 0 && errno != EACCES)
              continue;
            // insert the child's process id in background process ids
            pthread_mutex_lock(&lock);
            background_pids.insert(pid);
            pthread_mutex_unlock(&lock);
          }
        }
    }
    //Freeing the allocated memory 
    for(i=0;tokens[i]!=NULL;i++){
      free(tokens[i]);
    }
    free(tokens);

    fflush(stdout);
    if(toBreak)
      break;
  }

  exit(0);
}

void getfl(char **tokens,bool display)
{
    char *newtokens[6];
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
    filltokens(newtokens);
    int i = 1;
    while (tokens[i] != NULL)
    {
        strcpy(newtokens[1],tokens[i]);
        pid = fork();
        if(pid == -1){
          fprintf(stderr,"Fork Failed\n");
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
    filltokens(newtokens);
    int i = 1;
    while (tokens[i] != NULL)
    {
        strcpy(newtokens[1],tokens[i]);
        pid = fork();
        if(pid == -1){
          fprintf(stderr,"Fork Failed\n");
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
    strcpy(newtokens[0],file_path);
    if(strcmp(host,"") == 0 || strcmp(portno,"") == 0 ){
      fprintf(stderr,"Please use server command to specify hostname and portno\n");
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
          error("Exec failed");
      exit(0);
}

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

void error(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

void sigint_handler(int sig_no)
{   
    // sleep(0.1);
    while(waitpid(0,NULL,0)  > 0);
    printf("\n");
}

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
  int i = 0;
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
        break;
      }
    }
    i++;
  }
  
  if(i == 3 || k == -1){
    k = -1;
    printf("Expected getfl <file> [| or >] <file2/command..>\n");
  }
  return k;
}

void handle_getfl(int k,char **tokens)
{
  int p[2];
  int pid;
  switch(k){

    case 1:
      // close the stdout descriptor
      close(1);
      // open the file descriptor pointed by tokens[3] for writing which will be assigned 1
      if(open(tokens[3], O_WRONLY ) < 0){
        if (open(tokens[3],O_WRONLY | O_CREAT | O_EXCL) < 0)
          error("open failed");
      }
      getfl(tokens);
      break;

    case 0:
      getfl(tokens);
      break;
    
    case 2:
      // make p[0] the read end and p[1] the write end of a pipe
      if(pipe(p) == -1)
        error("pipe failed");
      
      pid = fork();
      if(pid < 0)
        error("fork");
      else if(pid == 0){
        // close the stdout descriptor
        close(1);
        // call dup to point p[1] (the write end of pipe) to the output descriptor
        dup(p[1]);
        close(p[0]);
        close(p[1]);
        getfl(tokens);
      }
      
      pid = fork();
      if(pid < 0)
        error("fork");
      else if(pid == 0){
        // close the stdin descriptor
        close(0);
        // call dup to p[0](read end) to point it to input descriptor
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
  switch(command_id){

    case 0:
        break;

    case 1:
        if (tokens[1] == NULL || tokens[2] != NULL)
        {
            fprintf(stderr,"Expected cd <directory>\n");
            return -1;
        }
        break;

    case 2:
        if (tokens[1] == NULL || tokens[2] == NULL || tokens[3] != NULL)
        {
            fprintf(stderr,"Expected server <server-IP> <server-port>\n");
            return -1;
        }
        break;

    case 3:
        if (tokens[1] == NULL)
        {
            fprintf(stderr,"Expected getfl <file>\n");
            return -1;
        }
        break;

    case 4:
        if (tokens[1] == NULL)
        {
            fprintf(stderr,"Expected getsq <file(s)>\n");
            return -1;
        }
        break;

    case 5:
        if (tokens[1] == NULL)
        {
            fprintf(stderr,"Expected getpl <file(s)>\n");
            return -1;
        }
        break;

    case 6:
        if (tokens[1] == NULL || tokens[2] != NULL)
        {
            fprintf(stderr,"Expected getbg <file>\n");
            return -1;
        }
        break;

    case 7:
        if (tokens[1] != NULL)
        {
            fprintf(stderr,"Expected exit (no arguments)\n");
            return -1;
        }
        break;

    default:
        break;
  }
  return 1;
}

void *kill_zombies(void *)
{
    pid_t w;
    int status;
    
    while(1){

        while (( w = waitpid(-1, &status, WNOHANG) ) > 0)
        {
            pthread_mutex_lock(&lock);
            if(background_pids.count(w) != 0){
              background_pids.erase(w);
              if (WIFEXITED(status)) 
                printf("Background process with pid %d completed\n",w);
              else if (WIFSIGNALED(status)) 
                fprintf(stderr,"Background process with pid %d exited due to errors!",w);
              printf("Press [ENTER] to continue\n");
            }
            pthread_mutex_unlock(&lock);
        }
        sleep(timeout);
    }
}
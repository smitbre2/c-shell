/********************************************************************
#Author: Brenden Smith
#Description: Small shell with a few built in commands:
#	exit, cd, and status (They are always run in the foreground).
#       Other shell functions will be achieved with exec fuctions.
#	Commands can be ran in the background given final argument &.
#
#	Ctrl-Z will activate foreground only mode.
*********************************************************************/

#include <sys/wait.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>	//For O_RDONLY and such
#include <unistd.h>

int mode = 0;				//Tracks foreground only mode
int read_flag = 0;				//Does std need to be read
int write_flag = 0;				//Write out std to file1
int status_flag = 0;


/*************************************************************
* Signal Handler: Will handle all SIGTSTP SIGINT and SIGCHLD
* ************************************************************/
void sig_handle(int sig){
   //Ctrl-C
   if(sig == SIGINT){
      status_flag = 2;
      signal(SIGINT, sig_handle);	//Reset to the signal handler

   }else if(sig == SIGTSTP){
	
       if(mode == 1){			//Set shell modes
	   mode = 0;
	   printf("Exiting foreground-only mode\n");
	   fflush(stdout);
	}
	else{
	   mode = 1;
	   printf("Entering foreground-only mode (& is now ignored)\n");
	   fflush(stdout);
	}
	signal(SIGTSTP, sig_handle);	//Reset to the handler

   }else if (sig == SIGCHLD){
   	pid_t pid;
	int sig_status;
     
	while ((pid = waitpid(-1, &sig_status, WNOHANG)) > 0){	//Check for ending background procs

	   if (sig_status > 0){
	      printf("background pid %d is done: termination signal %d\n", pid, sig_status);
	      fflush(stdout);
	      status_flag = sig_status;
	   }else{
	      printf("background pid %d is done: exit value %d\n", pid, sig_status);
	      fflush(stdout);
	      status_flag = sig_status;
	   }
	}
   }

}

/********************************************************************
#Description: Exit the shell.
#Parameters:
#Return: Will return 0 to end shell loop
********************************************************************/
int get_me_outta_here(){
   return 0;
}

/********************************************************************
#Description: Will query on the exit status of last command.
#Parameters:
#Return:
********************************************************************/
int status(){
   if (status_flag < 2) {
      printf("exit value %i\n", status_flag);
      fflush(stdout);
   }else{
      printf("terminated by signal %d\n", status_flag);
   }
   return 1;
}


/********************************************************************
#Description: Change directory.
#Parameters: cd, and a destination from the HOME variable
#Return: Returns 1
********************************************************************/
int cd(char **p){
   char buff[100];
 
   //Check arguments
   if(p[1] == NULL){
   	chdir(getenv("HOME"));	//CD to home directory
   	return 1;
   }
   
   //change to possible folder
   if( chdir(p[1]) != 0){
      perror("");
      fflush(stdout);
      
   }else{   
      getcwd(buff, 100);
      printf("%s\n", buff);
      fflush(stdout);
   }
   status_flag = 1;
   return 1;

}



/********************************************************************
#Description: Read input from the user and return it to the main loop
#Parameters: None
#Return: Will return success at end of file or newline. Failure on
	bad memory allocation.
********************************************************************/
char* read_line(){
   //Declare buffer and letter vars
   int buffer_size = 2048;
   char *buffer = malloc(sizeof(char) * buffer_size);
   int letter;
   char expan[6];
   int expan_count = 0;		//Will track for variable expansion
   int index = 0;
   int myp;


   //Pray for no failure
   if(buffer == NULL){
	exit(EXIT_FAILURE);
   }
   
   //Start reading each character into the buffer. Ends on EOF.
   do{
	letter = getchar();

	//Base case
	if (letter == EOF){
	   return NULL;
	}
	else if (letter == '\n'){	//If end of line, cap string and return
	   buffer[index] = '\0';
	   return buffer;
	}
	else if(letter == '$'){		//Check for special char
	   //Expand
	   expan_count++;
	   
	   if(expan_count == 2){	//If two in succession, expand var
	   	//Get pid
		myp = getpid();
		sprintf(expan, "%d", myp);	//Stringify
		
		//Insert pid and update position
	   	int i = 1;
		buffer[index-1] = expan[0];
		for(i;i < strlen(expan); i++){
			buffer[index] = expan[i];	//Insert into buffer
			index++;
		}
		expan_count = 0;			//Reset expansion count
	   
	   }else{
	      buffer[index] = letter;	//Write in $, might not be $$
	      index++;
	   }
	
	}else{				//Insert letter
	   buffer[index] = letter;
	   expan_count = 0;		//Reset variable expan tracker
	   index++;
	}

   
   }while(1);


}


/********************************************************************
#Description: Takes the stored user-input and seperates all provided
	arguments with the function.
#Parameters: User-input from read_line()
#Return: Seperated Parameters
********************************************************************/
char** get_params(char *u, int* back_check){
   char* word;
   char* next;
   char** p = malloc(sizeof(char*) * 512); 		//Holds params
   int i = 0;
   int getp;
      

   word = strtok(u, " ");
   if(!word){
   	return NULL;
   }
   
   if(strcmp(word, "#") == 0){			//Is it a comment
   	return NULL;
   }else if(word[0] == '#'){
   	return NULL;
   }
   while(word != NULL){				 //Input is good to read
       
       next = strtok(NULL, " ");		//Peak for end of input

       //If last param is actually & dont insert and update flags
       if(next == NULL){
	  if(strcmp(word, "&") == 0){		//If background command found
	        
	        if(mode == 0){
			*back_check = 1;	//Set flag if mode allows
		}else{
			*back_check = 0;
		}
	
          	return p;
	  }
       }

       if(strcmp(word, "<") == 0){
	  //Call readfile
       	  read_flag = 1;
       
       }else if(strcmp(word, ">") == 0){
       	  //Call writeFile
	  write_flag = 1;
       }

       //Write out word
       	p[i] = malloc(sizeof(char) * strlen(word));
       	strcpy(p[i], word);
	i++;
       	word = next;
       }
   
   fflush(stdout);
   return p;
}


/********************************************************************
#Description: Launches built-in functions.
#Parameters: User inputed parameters, input for compare is checked
#Return: Returns value from built-in's. 1/0
********************************************************************/
int built_in(char** p, int *back_check){

   *back_check = 0;
   if(strcmp(p[0], "cd") == 0){	
      return cd(p);
    }
    else if(strcmp(p[0], "exit") == 0){
    	return get_me_outta_here();	//Exit
    }
    else{
    	return status();		//Has to be the last one
    }
}


/********************************************************************
#Description:Handles forking off a child process and calling it
	through execvp, system() could have been used but pid handling
	would be harder.
#Parameters: Takes user parameters
#Return: status of child process
********************************************************************/
int staging(char** p, int* back_check){
   //Create a fork
   pid_t child;
   int status;
   int check;
   int errnum;
   int index = sizeof(p)/sizeof(p[0])-1;
   int n;


   child = fork();		//Create a forked proc



   if (child == -1){		//Catch error
      printf("Error forking child process\n");
      fflush(stdout);
      return 0;
   }
   else if (child == 0){	//Child proc
      execvp(p[0], p);
      perror("");
      exit(EXIT_FAILURE);   
   }
   else{			//Parent proc


      if(mode == 0 && *back_check == 1){	//Run in background
	 *back_check = 0;

	 printf("background pid is %i\n", child);
	 fflush(stdout);

	 return 0;
      }
      else{
	 n = waitpid(child, &status, 0);		//Wait for our buddy to finish

	 if (n != child){
	    //error
	    perror("");
	    fflush(stdout);
	 }

	 if (status > 2)
	    status_flag = 1;		//Error code
	 else if (status == 0)
	    status_flag = 0;

	 return status_flag;
      }

   }
   return 0;
}


/********************************************************************
#Description: Will process built in functions or call staging() to
fork a new process.
#Parameters: Parameters from user-input
#Return: 0 on exit command, 1 otherwise
 ********************************************************************/
int execute(char** p, int* last, int* back_check ){
   const char * easy_string[] = {"cd", "exit", "status", "#"}; 


   //Any input at all?
   if(p == NULL)
      return 1;

   //Check fo built in commands
   if(p[0]){
      int i = 0;
      for(i; i < 4; i++){	 

	 if(strcmp(p[0], easy_string[i]) == 0){		//Check for built-in
	    //Return the function
	    if (i < 3)
	       return (built_in(p, back_check));
	    else
	       return 1;				//Is comment
	 }
      }
   }
   
   //Check if <, > was flagged
   if(read_flag == 1 || write_flag == 1){
      read_flag = 0;
      write_flag = 0;
      return write_file(p);
   }

   //Otherwise prep for forkin
   *last = staging(p, back_check);
   return 1;
}



/********************************************************************
#Description: Free up the heap.
#Parameters: user input and seperated parameters
#Return: None
 ********************************************************************/
void cleanup(char *u, char **p){
   if (u != "")
      free(u);

   printf("Passed p free");
   int i = 0;
   int j = 0;
   int count = 0;

   while(p[j] != NULL){
      j++;
   }
   for(i; i < j; i++){	//Free each parameter stored
      free(p[i]);
   }
   free(p);
}


/******************************************************************
#Will handle > or <  when passed as input.
#Write input out to a file
 ******************************************************************/
int write_file(char** p){
   char command[512];
   int i = 0;
   int system_status;
   memset(command, 0, strlen(command));

   while(p[i] != NULL) {
      strcat(command, p[i]);	//Create sys call
      strncat(command, " ", 1);
      i++;
   }

   system_status = system(command);		//Pass the duties elsewhere
   if (system_status > 0)
      status_flag = 1;
   else
      status_flag = 0; 
   return 1;
}



/********************************************************************
#Description: Main loop for recieving shell commands.
 ********************************************************************/
void smallsh(){

   char *input;
   char **params;
   int check = 1;
   int last = 80;

   int back_check = 0;		//Will track Background flags i.e. '&'

   int i = 0;
   int tmp = 0;
   int k = 0;



   //Get user input until execute returns a success.
   signal(SIGINT, sig_handle);
   signal(SIGTSTP, sig_handle);
   signal(SIGCHLD, sig_handle);
   do{
      //reap();		//Check for zombie procs


      printf(": ");
      fflush(stdout);

      //Read the line and decipher arguments
      input = read_line();

      params = get_params(input, &back_check); 

      //Execute the provided arguements.
      if(execute(params, &last, &back_check) == 0){	
	 check = 0;		//Exit the shell as user-input directed
      }


   }while(check); 
}


int main(){

   //Call the shell loop
   smallsh();

   return EXIT_SUCCESS;
}


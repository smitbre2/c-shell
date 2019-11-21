/********************************************************************
#Author: Brenden Smith
#Description: Small shell that will have a few built in commands:
#	exit, cd, and status (They are always run in the foreground).
#       Other shell functions will be achieved with exec. Commands
#	Can be ran in the background given final argument &.
#
#	Ctrl-Z will activate foreground only mode.
*********************************************************************/

#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

//Mode will altered by sig_handle to change to foreground process only
int mode = 0;
int back_check = 1;

//Signal Handler: Will handle all SIGTSTP and SIGINT
void sig_handle(int sig){
   //Ctrl-C
   if(sig == SIGINT){
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
   }

  //Report the halted process
  if(sig == SIGCHLD){
     printf("terminated by signal %i\n", sig);
     fflush(stdout);
     signal(SIGCHLD, sig_handle);
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
int status(char **p, int *status){
   printf("exit value %i\n", *status);
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
   int expan_count;		//Will track for variable expansion
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
	   if(buffer[index-1] == '&'){	//Set background flag
	      back_check = 0;
	      buffer[index-1] = '\0';
	   }
	   else{
	      back_check = 1;
	   }

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
	   
	   }else{
	      buffer[index] = letter;	//Write in $, might not be $$
	   }

	}
	else{				//Insert letter
	   buffer[index] = letter;
	   expan_count = 0;		//Reset variable expan tracker
	}

	index++;			//Increment buffer index
   
   }while(1);

}


/********************************************************************
#Description: Takes the stored user-input and seperates all provided
	arguments with the function.
#Parameters: User-input from read_line()
#Return: Seperated Parameters
********************************************************************/
char** get_params(char *u){
   char* word;
   char** p = malloc(sizeof(char*) * 512); 		//Holds params
   int i = 0;
   int getp;
   char myp[6];

   word = strtok(u, " ");
   while(word != NULL){				 //Input is good to read
      
      p[i] = malloc(sizeof(char) * strlen(word));//Allocate for param size
      strcpy(p[i], word);

      i++;					//Move iterator
      word = strtok(NULL, " ");			//Get next word
   
      
   }
   return p;
}


/********************************************************************
#Description: Launches built-in functions.
#Parameters: User inputed parameters, input for compare is checked
#Return: Returns value from built-in's. 1/0
********************************************************************/
int built_in(char** p, int* last){
   if(strcmp(p[0], "cd") == 0){	
      return cd(p);
    }
    else if(strcmp(p[0], "exit") == 0){
    	return get_me_outta_here();
    }
    else{
    	return status(p, last);		//Has to be the last one
    }
}


/********************************************************************
#Description:Handles forking off a child process and calling it
	through execvp, system() could have been used but pid handling
	would be harder.
#Parameters: Takes user parameters
#Return: status of child process
********************************************************************/
int staging(char** p){
   //Create a fork
   pid_t child;
   int status;
   int check;
   int errnum;
   int index = sizeof(p)/sizeof(p[0])-1;
   int back_check;

   int n;

   child = fork();		//Create a forked proc



   if (child == -1){		//Catch error
      perror("");
      fflush(stdout);
      return 0;
   }
   else if (child == 0){	//Child proc
        execvp(p[0], p);
        perror("");
	fflush(stdout);
        return EXIT_FAILURE;
   }
   else{			//Parent proc
      if(mode == 0 && back_check == 0){	//Run in background
	 return 0;
      }
      else{
      	n = waitpid(child, &status,0);

	if (n != child){
		//error
		perror("");
		fflush(stdout);
	}else if (WIFSIGNALED(status)){		//Catch signal stop
		//Caught signal
		printf("Terminated by: %i", WTERMSIG(status));
		fflush(stdout);
	}else if (WIFEXITED(status)) {		//Catch termination
		//Done
		printf("Terminated by: %i", WTERMSIG(status));
		fflush(stdout);
	}
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
int execute(char **p, int *last){
   const char * easy_string[] = {"cd", "exit", "status", "#"}; 
   
   //Check fo built in commands
   if(p[0]){
      int i = 0;
      for(i; i < 4; i++){	 
	 
	 if(strcmp(p[0], easy_string[i]) == 0){		//Check for built-in
	    //Return the function
	    if (i < 3)
	       return (built_in(p, last));
	    else
	       return 1;				//Is comment
	 }
      }
   }

   //Any input at all?
   if(p[0] == NULL)
      return 1;

   //Otherwise prep for forkin
   *last = staging(p);
   return 1;
}



/********************************************************************
#Description: Free up the heap.
#Parameters: user input and seperated parameters
#Return: None
********************************************************************/
void cleanup(char *u, char **p){
	free(u);

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

/********************************************************************
#Description: Main loop for recieving shell commands.
********************************************************************/
void smallsh(){

   char *input;
   char **params;
   int check = 1;
   int last = 80;

   int i = 0;
   //Get user input until execute returns a success.
   signal(SIGINT, sig_handle);
   signal(SIGTSTP, sig_handle);
   do{
      	printf(": ");
      	fflush(stdout);
      
      	//Read the line and decipher arguments
      	input = read_line();

        params = get_params(input);
     
      //Execute the provided arguements.
      if(execute(params, &last) == 0){	
	 	check = 0;		//Exit the shell as user-input directed
      	}
   }while(check); 
   cleanup(input, params);   //Cleanup memory so computers don't hate me
}


int main(){

   //Call the shell loop
   smallsh();
	
   return EXIT_SUCCESS;
}

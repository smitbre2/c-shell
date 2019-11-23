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

struct child_pids{
   int pids[10];
   int num_pids;
};


int mode = 0;				//Tracks foreground only mode
int read = 0;				//Does std need to be read
int write = 0;				//Write out std to file1


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
   fflush(stdout);
   return 1;
}


/********************************************************************
#Description: Change directory.
#Parameters: cd, and a destination from the HOME variable
#Return: Returns 1
********************************************************************/
int cd(char **p){
   char buff[100];
 
   //Check argumen:ts
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
	
	}else{				//Insert letter
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
       	  read = 1;
       
       }else if(strcmp(word, ">") == 0){
       	  //Call writeFile
	  write = 1;
       }

       //Write out word
       	p[i] = malloc(sizeof(char) * strlen(word));
       	strcpy(p[i], word);
       	i++;
       	word = next;
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
int staging(char** p, int* back_check, struct child_pids* kids){
   //Create a fork
   pid_t child;
   int status;
   int child_pid;
   int check;
   int errnum;
   int index = sizeof(p)/sizeof(p[0])-1;

   int n;

   child = fork();		//Create a forked proc



   if (child == -1){		//Catch error
      perror("");
      fflush(stdout);
      return 0;
   }
   else if (child == 0){	//Child proc
        child_pid = getpid();
	if(mode == 0 && *back_check == 1)
		printf("background pid is %d\n", child_pid);
	execvp(p[0], p);
        perror("");
	fflush(stdout);
	return 0;
   }
   else{			//Parent proc
      
      
      if(mode == 0 && *back_check == 1){	//Run in background
	 *back_check = 0;

	 fflush(stdout);
	 return 0;
      }
      else{
      	n = waitpid(child, &status,0);		//Wait for our buddy to finish

	if (n != child){
		//error
		perror("");
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
int execute(char** p, int* last, int* back_check, struct child_pids* kids){
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
	       return (built_in(p, last));
	    else
	       return 1;				//Is comment
	 }
      }
   }

   //Check if <, > was flagged
   if(read == 1){
      read_file(p);
      read = 0;
      return 1;
   }else if (write == 1){
      write_file(p);
      write = 0;
      return 1;
   }

   //Otherwise prep for forkin
   *last = staging(p, back_check, kids);
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
#Will handle input when < is passed as a parameter.
#Read out the file
********************************************************************/
int read_file(char** p){
/*   char content[500];
   int i;
   int status;
  FILE *f = popen(p, "r");

   if(f == NULL){
      printf("Cannot open %s for input.\n", p[2]);
      fflush(stdout);
   }

   while(fgets(content, 500, f) != NULL){
      printf("%s", content);
      fflush(stdout);
   }

   status = pclose(f);
   if(status == -1){
   	printf("Error\n");
	fflush(stdout);
   }else{
   	
   }
   return 1;

   while(p[i] != NULL){
      strcat(content, p[i]);
      i++;
   }
   system(content);*/
   printf("Cannot open %s for input.\n", p[2]);
   return 1;
}
/******************************************************************
#Will handle > when passed as input.
#Write input out to a file
******************************************************************/
int write_file(char** p){

   char command[500];
   int i = 0;


   while(p[i] != NULL){
      strcat(command, p[i]);	//Concatanate command for sys call
      i++;
   }

   system(command);
   return 1;
}



/********************************************************************
#Description: Main loop for recieving shell commands.
********************************************************************/
void smallsh(){

   pid_t zombie;
   int status;
   char *input;
   char **params;
   int check = 1;
   int last = 80;

   struct child_pids kids;			//Holds all background pids
   int back_check = 0;		//Will track Background flags i.e. '&'

   int i = 0;
   int tmp = 0;
   int k = 0;
   //Get user input until execute returns a success.
   signal(SIGINT, sig_handle);
   signal(SIGTSTP, sig_handle);
   do{

      printf(": ");
      fflush(stdout);

      //Read the line and decipher arguments
      input = read_line();

      params = get_params(input, &back_check);

      //Execute the provided arguements.
      if(execute(params, &last, &back_check, &kids) == 0){	
	 check = 0;		//Exit the shell as user-input directed
      }

      for(k; k < 10; k++){		//Get any dead procs

	 if( (zombie = waitpid(-1, &status, WNOHANG)) > 0){
	    //A Child proc has finished
	    printf("background pid %d is done: exit value %d\n", kids.pids[k], WEXITSTATUS(status));
	    fflush(stdout);
	    kids.pids[k] = 0;
	    tmp++;
	 }
      }
      kids.num_pids -= tmp;		//Update Struct
   }while(check); 
   //cleanup(input, params);   //Cleanup memory so computers don't hate me
}


int main(){

   //Call the shell loop
   smallsh();

   return EXIT_SUCCESS;
}

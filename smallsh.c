#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>


// Using global variables for status 
// Approved by Brewster!
// https://edstem.org/us/courses/37585/discussion/3138126?answer=7185686
int globalStatus = 0;


// Using global variable for sigSTP
// Approved by Brewster!
// https://edstem.org/us/courses/37585/discussion/3120866?comment=7147775
int ignoreSIGTSTP = 0;


// This struct holds each separate element of the 
// command prompt
struct commandPrompt{

	char *command;
	// following 2.5 lecture
	char *arg[512];
	char *input;
	char *output;
	int background;
	int numArgs;

};

// This signal handler handles switching the global variable 
// to either accept a background indicator or not
void handle_SIGTSTP(int signo){
	

	// If the global varibale is toggled off, then turn it back on
	if(ignoreSIGTSTP == 0){
		char* message1 = "\nEntering foreground-only mode (& is now ignored)\n";
		write(STDOUT_FILENO, message1, 50);
		ignoreSIGTSTP = 1;

	}

	// vice versa, if toggled on, turn it off
	else if (ignoreSIGTSTP == 1){
		char* message2 = "\nExiting foreground-only mode\n";
		write(STDOUT_FILENO, message2, 30);
		ignoreSIGTSTP = 0;

	}

}

// This struct function tokenizes the string that the user entered and then 
// stores different elements such as commands, arguments, etc into their respective 
// commandPrompt struct members. 
struct commandPrompt *createPrompt(char* lineEntered){

	// Allocate memory for processing the current prompt
	struct commandPrompt *currPrompt = malloc(sizeof(struct commandPrompt));

	int i = 0;
	int inputChar = 0; // this variable detects if there is an input symbol
	int outputChar = 0; // this variable detects if there is an output symbol

	// set variables in command prompt struct
	currPrompt->background = 0; // this variable detects if there is an background symbol
	currPrompt->input = NULL;
	currPrompt->output = NULL;
	currPrompt->numArgs = 0;

	// Before we do anything, lets do PID expansion
	
	for(i = 0; i < strlen(lineEntered); i++){

		// sees the first $
		if(lineEntered[i] == '$'){

			// check to see if there is sequential $
			if(lineEntered[i + 1] == '$'){

				// found $$! Lets malloc an empty string to be inserted later
				char* pidString = malloc(2049 * sizeof(char));
				memset(pidString, '\0',sizeof(pidString));

				// first, replace the first character with %
				lineEntered[i] = '%';

				// then, replace the second character with i
				i++;
				lineEntered[i] = 'i';

				// get pid
				int insertPID = getpid();

				// This results in a %i, in which we can insert a variable inside through sprintf
				sprintf(pidString, lineEntered, insertPID);

				// transfer the pidString back into lineEntered
				strcpy(lineEntered, pidString);

				// free it!
				free(pidString);

			}
		}
	}

	// If the toggle for blocking SIGTSTP (background processes) is off, 
	// Then we can check if the command is being run in the background. 
	// Else, ignore it and run everything foreground
	if(ignoreSIGTSTP == 0){

		// check for background &
		char backgroundCheck = lineEntered[strlen(lineEntered) - 1];

		if (backgroundCheck == '&'){
			currPrompt->background = 1;
		}

	}
	

	// to use with strtok_r, used for the main line
    char *saveptr;

    // 1st token is command
    char *token = strtok_r(lineEntered, " ", &saveptr);
    currPrompt->command = calloc(strlen(token) + 1, sizeof(char));
    strcpy(currPrompt->command, token);

    // 2nd token is arguments, go through potential 512 arguments
    for (i = 0; i < 512; i++){

    	// The arguments are separated by spaces or \n
    	token = strtok_r(NULL, " ", &saveptr);

    	// This means we reached the end of the line prompt
	    if (token == NULL){
	    	break;
	    }

	    // input
	    else if (strcmp(token, "<") == 0){
	    	inputChar = 1;
	    	break;
	    }
	    
	    // output 
	    else if (strcmp(token, ">") == 0){
	    	outputChar = 1;
	    	break;
	    }

	    // background
	    else if(strcmp(token, "&") == 0){
	    	break;
	    }

	    else{
	    	currPrompt->numArgs++;
	    	currPrompt->arg[i] = token;

	    }	    
	
	}

	// third and fourth token is input/output file 

	// if < came first (input)
	if(inputChar == 1){

		// store input_file
		token = strtok_r(NULL, " ", &saveptr);
		currPrompt->input = calloc(strlen(token) + 1, sizeof(char));
    	strcpy(currPrompt->input, token);

    	// check for an output
    	token = strtok_r(NULL, " ", &saveptr);


    	// if the tokenizing results in a NULL, skip
    	if(token != NULL){

	    	if(strcmp(token, ">") == 0){

	    		token = strtok_r(NULL, " ", &saveptr);
				currPrompt->output = calloc(strlen(token) + 1, sizeof(char));
	    		strcpy(currPrompt->output, token);

	    	}
	    }
	}

	// if > came first (output)
	if(outputChar == 1){

		// store output_file
		token = strtok_r(NULL, " \n", &saveptr);
		currPrompt->output = calloc(strlen(token) + 1, sizeof(char));
    	strcpy(currPrompt->output, token);

    	// check for an input
    	token = strtok_r(NULL, " ", &saveptr);

    	if(token != NULL){
	    	if(strcmp(token, "<") == 0){
	    		token = strtok_r(NULL, " \n", &saveptr);
				currPrompt->input = calloc(strlen(token) + 1, sizeof(char));
	    		strcpy(currPrompt->input, token);
	    	}
	    }

	}
    
	return currPrompt;
}



// This function processes the command prompt struct and will handle
// background and foreground processes. The function will also take 
// in an array of background PIDS to store, which will be used later to 
// clean up zombie processes. 
int processPrompt(struct commandPrompt *currPrompt, int storePIDS[]){

	
	// First we check for built-in commands
	// exit
	if (strcmp(currPrompt->command, "exit") == 0){
		return 0;
	}
	
	// cd
	else if(strcmp(currPrompt->command, "cd") == 0){

		// Using this resrouce to get to home
		// https://stackoverflow.com/questions/9493234/chdir-to-home-directory
		if(currPrompt->numArgs == 0){

			char* HOME = getenv("HOME");
			chdir(HOME);

			return 1;
		
		}

		// else if there are anything in the arguments
		// change the directory from the first argument.  
		else if (currPrompt->numArgs > 0){
			chdir(currPrompt->arg[0]);

			return 1;


		}
	}

	// Check for status 
	else if(strcmp(currPrompt->command, "status") == 0){


		// Following Exploration: Process API - Monitoring Child Processes
		// Using MACROS 
		// Check if it exited normally
		if(WIFEXITED(globalStatus)){
			printf("exit value %d\n", WEXITSTATUS(globalStatus));
			fflush(stdout);
		}

		// else, its probably terminated 
		else{
			printf("terminated by signal %d\n", WTERMSIG(globalStatus));
			fflush(stdout);
		}

		return 1;
		
	}


	// if its none of these, then we need to execvp, fork, and waitpid foreground/background processes
	else{

		// From 3.1 Processes Lecture 
		pid_t spawnPID = -5;

		spawnPID = fork();
		
		switch(spawnPID){

			// when fork fails, it returns -1
			case -1: {
				perror(" Fork() error!\n");

				
				exit(1);
				break;
			}

			// when fork succeses, returns 0, child will process
			case 0: {

				// Exploration: Signal Handling API
				// ignores any children for SIGTSTP
				struct sigaction SIGTSTP_ignore = {0};
				SIGTSTP_ignore.sa_handler = SIG_IGN;
				sigaction(SIGTSTP, &SIGTSTP_ignore, NULL);

				// Also from: Exploration: Signal Handling API
				// Checks for background command exists, and if we are not ignoring background
				if(currPrompt->background == 1 && ignoreSIGTSTP == 0){
				
					// If we are running in the background,
					// Then we are ignoring SIGTSTP
					struct sigaction ignore_action2 = {0};				
					ignore_action2.sa_handler = SIG_IGN;
					sigaction(SIGTSTP, &ignore_action2, NULL);


				}

				else{

					// handles SIGINT to do default action in children
					struct sigaction default_action = {0};
					default_action.sa_handler = SIG_DFL;
					sigaction(SIGINT, &default_action, NULL);


				}
				


				int result = 0;

				// check for background, then redirect to NULL
				if(currPrompt->background == 1 && ignoreSIGTSTP == 0){

					// Redirecting source (stdin)
					int sourceFD = open("/dev/null", O_RDONLY);
                    if(sourceFD == -1){
                        perror("source open()");
                        
                        exit(1);
                        break;
                    }

                   
                    result = dup2(sourceFD, 0);
                    if(result == -1){
                        perror("source dup2()");
                        
                        exit(2);
                        break;

                    }

                    // Redirecting target (stdout)
                    int targetFD = open("/dev/null", O_WRONLY, 0644);
                    if (targetFD == -1){
                        perror("target open()");
                        
                        exit(1);
                        break;
                    }

                    result = dup2(targetFD, 1); 
                    if (result == -1){
                        perror("target dup2()");
                        
                        exit(2);
                        break;
                    }

				}
				

				// input/output redirection for standard foreground processes
                // Following Exploration: Processes and I/O
                // first, check for input redirection < 
                
                if(currPrompt->input != NULL){
                    // open source file
                    int sourceFD = open(currPrompt->input, O_RDONLY);
                    if(sourceFD == -1){
                    	
                        perror("source open()");
                        exit(1);
                    }

                    // redirect stdin to source file
                    result = dup2(sourceFD, 0);
                    if(result == -1){
                    	
                        perror("source dup2()");
                        exit(2);
                    }

                    // Close
                    fcntl(sourceFD, F_SETFD, FD_CLOEXEC);

                }

                // second, check for output redirection > 
                if(currPrompt->output != NULL){

                    int targetFD = open(currPrompt->output, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (targetFD == -1){
                       
                        perror("target open()");
                        
                        exit(1);
                    }

                    // redirect stdout to source file
                    result = dup2(targetFD, 1); 
                    if (result == -1){
                       
                        perror("target dup2()");
                        
                        exit(2);
                    }

                    // close
                    fcntl(targetFD, F_SETFD, FD_CLOEXEC);

                }

				// this is the "actual" length of the command line input
				// command + arguments
				int commandPlusArg = currPrompt->numArgs + 1;

				// create an array of pointer strings that is equal
				// to the command line input
				char* args[commandPlusArg];

				int i = 0;

				for (i = 0; i < commandPlusArg + 1; i++){

					// get that first command and append it to the beginninng
					if (i == 0){

						args[i] = currPrompt->command;

					}

					// This means that we reached the end of the arguments, and assign 
					// it to NULL to ensure formating for execvp()
					else if(i == (currPrompt->numArgs) + 1){

						args[i] = NULL;

					}

					else{

						// This indicates that there are no arguments
						if(currPrompt->numArgs == 0){
							args[i] = NULL;
							break;
						}

						// Else assign the argument to the array
						else{
							args[i] = currPrompt->arg[i-1];
						}

					}
				}


				// run by execvp
				if (execvp(args[0], args) < 0){

					
					perror("Exec failure");
					
					exit(1);

				}

			
				exit(2);
				break;

			}


			// this will be the parent running, is the PID
			default: {

				// If this process is run in the foreground
				if (currPrompt->background == 0){

					// wait
					spawnPID = waitpid(spawnPID, &globalStatus, 0);

					// check if it got terminated by a signal 
					if(!WIFEXITED(globalStatus)){
						printf("terminated by signal %d\n", WTERMSIG(globalStatus));
						fflush(stdout);
					}

					break;
				}

			
				// If this process is run in the background
				else if (currPrompt->background == 1){
					
					// print background pid
					printf("background pid is %i\n", spawnPID);
					fflush(stdout);
				
					// And then store the pid into the array to check for finished process later
					int j = 0;
					for(j = 0; j < 256; j++){

						if(storePIDS[j] == 0){
							storePIDS[j] = spawnPID;
							break;
						}

					}


					break;
				}

			}	

		}


	}

	return 1;
}


int main() {

	// From Exploration Code
	// This ignores sigint in the main function parent
	struct sigaction ignore_action = {0};
	// ignore_action struct as SIG_IGN as its signal hanlder
	ignore_action.sa_handler = SIG_IGN;
	// Then register the ignore_action as handler for SIGINT
	// This will be ignored in main shell
	sigaction(SIGINT, &ignore_action, NULL);


	// Handling SIGTSTP
	// Fill out the SIGTSTP_action struct 
	struct sigaction SIGTSTP_action = {0};
	SIGTSTP_action.sa_handler = handle_SIGTSTP;
	sigfillset(&SIGTSTP_action.sa_mask);
	// No flags set
	SIGTSTP_action.sa_flags = 0;
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);


	// initialize the inputType variable
	// Will handle reinput of prompts
	int inputType = 1;

	int storePIDS[256];

	int j = 0;
	for (j = 0; j < 256; j++){
		storePIDS[j] = 0;
	}


	while(inputType == 1){
		

		int i = 0;
		int checkStatus;
		pid_t checkPID;

		// handling printing backgorund message

		for (i = 0; i < 256; i++){

			// find a background 
			if(storePIDS[i] != 0){
				

				checkPID = waitpid(storePIDS[i], &checkStatus, WNOHANG);

				if(checkPID > 0){
				
					// use macros inside
					if(WIFEXITED(checkStatus)){

						printf("background pid %d is done: exit value %d\n", checkPID, WEXITSTATUS(checkStatus));
						fflush(stdout);
						storePIDS[i] = 0;

					}

					if(WIFSIGNALED(checkStatus)){

						printf("background pid %d is done: terminated by signal %d\n", checkPID, WTERMSIG(checkStatus));
						fflush(stdout);
						storePIDS[i] = 0;

					}



				}

			}

		}

		// Seen from 2.4 File Access in C
		size_t bufferSize = 2049; // 2048 + 1 for the null character
		char* lineEntered = NULL; // buffer
		int numCharsEntered = -5;

		// since buffersize isn't 0, we have to allocate the buffer ourselves to 
		// handle the max amount of characters in one allocation
		// https://man7.org/linux/man-pages/man3/getline.3.html
		lineEntered = malloc(bufferSize * sizeof(char));
		memset(lineEntered, '\0',sizeof(lineEntered));


		printf(": ");
		fflush(stdout);

		// // get input
		numCharsEntered = getline(&lineEntered, &bufferSize, stdin);
		if (numCharsEntered == -1){
			clearerr(stdin);
		}

		else{

			// From 2.4 file access
			numCharsEntered--;
			lineEntered[numCharsEntered] = '\0';

			// first, check for comments and blanks
			// The reason for the 1 is that it is counting newLine
			// So an "empty" input would be just 1 character an a Newline
			if(lineEntered[0] == '#' || (numCharsEntered == 0 && lineEntered[0] == '\0')){
				free(lineEntered);
				continue;
			}

			// fill the prompt struct
			struct commandPrompt *newPrompt = createPrompt(lineEntered);

			inputType = processPrompt(newPrompt, storePIDS);

			// free memory
			free(lineEntered);
			free(newPrompt->command);
			if (newPrompt->input != NULL){
				free(newPrompt->input);
			}
			if (newPrompt->output != NULL){
				free(newPrompt->output);
			}
			free(newPrompt);


		}


	}


	return EXIT_SUCCESS;

}

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


struct commandPrompt{

	char *command;
	// following 2.5 lecture
	char *arg[512];
	char *input;
	char *output;
	int background;
	int numArgs;

};


void handle_SIGTSTP(int signo){
	
	if(ignoreSIGTSTP == 0){
		char* message1 = "\nEntering foreground-only mode (& is now ignored)\n";
		write(STDOUT_FILENO, message1, 50);
		ignoreSIGTSTP = 1;

	}

	else if (ignoreSIGTSTP == 1){
		char* message2 = "\nExiting foreground-only mode\n";
		write(STDOUT_FILENO, message2, 30);
		ignoreSIGTSTP = 0;

	}

}

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

	    else if (strcmp(token, "<") == 0){
	    	inputChar = 1;
	    	break;
	    }
	    	
	    else if (strcmp(token, ">") == 0){
	    	outputChar = 1;
	    	break;
	    }

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



int processPrompt(struct commandPrompt *currPrompt, int pidArray[]){

	
	// Received help from Patrick Iacob
	// check for finished background processes, 
	// int pid = waitpid(-1, &status_code, WNOHANG); // return any processes 
	// do this in a do while loop until pid is greater than 0
	
	// pid_t pidCheck;
	// int status_code;

	// do{

	// 	pidCheck = waitpid(-1, &status_code, WNOHANG);
		
	// 	if(pidCheck > 0){

			
	// 		if(status_code > 2){
	// 			printf("background %i is done: terminated by signal %i\n", pidCheck, status_code);
	// 			fflush(stdout);
	// 			break;
	// 		}

	// 		printf("background %i is done: exit value: %i\n", pidCheck, status_code);
	// 		fflush(stdout);				

	// 	}

	// }while (pidCheck > 0);


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

		else if (currPrompt->numArgs > 0){
			chdir(currPrompt->arg[0]);

			return 1;


		}
	}

	else if(strcmp(currPrompt->command, "status") == 0){


		// Following Exploration: Process API - Monitoring Child Processes
		if(WIFEXITED(globalStatus)){
			printf("exit value %d\n", WEXITSTATUS(globalStatus));
			fflush(stdout);
		}

		else{
			printf("terminated by signal %d\n", WTERMSIG(globalStatus));
			fflush(stdout);
		}

		return 1;
		
	}



	// if its none of these, then we need to execvp, fork, and waitpid 
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


				// ignores any children for SIGTSTP
				struct sigaction SIGTSTP_ignore = {0};
				SIGTSTP_ignore.sa_handler = SIG_IGN;
				sigaction(SIGTSTP, &SIGTSTP_ignore, NULL);

				
				// also from exploration
				// Checks for background exists, and if we are not ignoring background
				if(currPrompt->background == 1 && ignoreSIGTSTP == 0){
					
					// From Exploration Code
					// This ignores sigint for childeren in background
					struct sigaction ignore_action2 = {0};

					// ignore_action struct as SIG_IGN as its signal hanlder
					ignore_action2.sa_handler = SIG_IGN;

					// Then register the ignore_action as handler for SIGINT
					// This will be ignored in main shell
					sigaction(SIGTSTP, &ignore_action2, NULL);


				}

				else{

					// handles SIGINT
					struct sigaction default_action = {0};
					default_action.sa_handler = SIG_DFL;
					sigaction(SIGINT, &default_action, NULL);


				}
				


				int result = 0;
				// check for background, then redirect to NULL

				if(currPrompt->background == 1 && ignoreSIGTSTP == 0){

					int sourceFD = open("/dev/null", O_RDONLY);
                    if(sourceFD == -1){
                        perror("source open()");
                        
                        exit(1);
                        break;
                    }

                    // redirect stdin to source file
                    result = dup2(sourceFD, 0);
                    if(result == -1){
                        perror("source dup2()");
                        
                        exit(2);
                        break;

                    }


                    int targetFD = open("/dev/null", O_WRONLY, 0644);
                    if (targetFD == -1){
                        perror("target open()");
                        
                        exit(1);
                        break;
                    }

                    // redirect stdout to source file
                    result = dup2(targetFD, 1); 
                    if (result == -1){
                        perror("target dup2()");
                        
                        exit(2);
                        break;
                    }

				}
				

				// input/output redirection
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
					else if(i == (currPrompt->numArgs) + 1){

						args[i] = NULL;

					}

					else{

						if(currPrompt->numArgs == 0){
							args[i] = NULL;
							break;
						}

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

				if (currPrompt->background == 0){


					spawnPID = waitpid(spawnPID, &globalStatus, 0);

					if(!WIFEXITED(globalStatus)){
						printf("terminated by signal %d\n", WTERMSIG(globalStatus));
						fflush(stdout);
					}

					break;
				}

				// What I did so far
				// If there is a background indicator, no need to waitpid
				// make sure 
				// /dev/null
				else if (currPrompt->background == 1){
					

					
					printf("background pid is %i\n", spawnPID);
					fflush(stdout);
					

					

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


	int inputType = 1;

	int pidArray[256];

	while(inputType == 1){
		

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

			
			inputType = processPrompt(newPrompt, pidArray);



			

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

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <string.h>



struct commandPrompt{

	char *command;
	// following 2.5 lecture
	char *arg[512];
	char *input;
	char *output;
	int background;

};


struct commandPrompt *createPrompt(char* lineEntered){


	// Allocate memory for processing the current prompt
	struct commandPrompt *currPrompt = malloc(sizeof(struct commandPrompt));

	int numArgs = 0; // This variable counts for the amount of arguments processed
	int inputChar = 0; // this variable detects if there is an input symbol
	int outputChar = 0; // this variable detects if there is an output symbol

	// set variables in command prompt struct
	currPrompt->background = 0; // this variable detects if there is an background symbol
	currPrompt->input = NULL;
	currPrompt->output = NULL;


	// check for background &
	char backgroundCheck = lineEntered[strlen(lineEntered) - 1];

	if (backgroundCheck == '&'){
		currPrompt->background = 1;
	}

	// to use with strtok_r, used for the main line
    char *saveptr;

    // 1st token is command
    char *token = strtok_r(lineEntered, " ", &saveptr);
    currPrompt->command = calloc(strlen(token) + 1, sizeof(char));
    strcpy(currPrompt->command, token);

    // 2nd token is arguments, go through potential 512 arguments
    int i = 0;
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
	    	numArgs++;
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

	    		printf("%s\n", token);
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

	// test print

	// command
	printf("command: %s\n", currPrompt->command);

	// loop through args
	for (i = 0; i < numArgs; i++){

		if(currPrompt->arg[i] != NULL){

			printf("arg %d: %s\n", i, currPrompt->arg[i]);

		}
	}

	if(currPrompt->input != NULL){
		printf("input: %s\n", currPrompt->input);
	}

	if(currPrompt->output != NULL){

		printf("output: %s\n", currPrompt->output);
	}

	printf("background: %d\n", currPrompt->background);

    
	return currPrompt;
}


int main() {


	int inputType = 1;


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

		numCharsEntered = getline(&lineEntered, &bufferSize, stdin);
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

		inputType = 0;

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

	return EXIT_SUCCESS;

}

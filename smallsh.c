#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <string.h>



struct commandPrompt{

	char *command;
	char **arg;
	char *input;
	char *output;
	char *exp;

};


struct commandPrompt *createPrompt(char* lineEntered){

	// Allocate memory for processing the current prompt
	struct commandPrompt *currPrompt = malloc(sizeof(struct commandPrompt));

	// to use with strtok_r, used for the main line
    char *saveptr;

    // 1st token is command
    char *token = strtok_r(lineEntered, " ", &saveptr);
    currPrompt->command = calloc(strlen(token) + 1, sizeof(char));
    strcpy(currPrompt->command, token);

    // https://www.ibm.com/docs/en/i/7.4?topic=functions-strtok-tokenize-string
    // 2nd token is arguments, go through potential 512 arguments
    currPrompt->arg = malloc(512 * sizeof(char*));

    int i = 0;
    for (i = 0; i < 512; i++){

    	// The arguments are separated by spaces or \n
    	token = strtok_r(NULL, " \n", &saveptr);

	    if (token == NULL){
	    	break;
	    }

	    
	    if (strcmp(token, "<") == 0){
	    	break;
	    }

	    else{
	    	// printf("%s\n", token);

	    	currPrompt->arg[i] = calloc(strlen(token) + 1, sizeof(char));
	    	strcpy(currPrompt->arg[i], token);
	    }	    
	
	}


	for (i = 0; i < 512; i++){


		if(currPrompt->arg[i] != NULL){

			printf("%s\n", currPrompt->arg[i]);
		}
	}



    
	return currPrompt;
}


int main() {


	int inputType = 1;

	// Seen from 2.4 File Access in C
	int numCharsEntered = -5;
	size_t bufferSize = 2049; // 2048 + 1 for the null character
	char* lineEntered = NULL; // buffer
	
	while(inputType == 1){

		// since buffersize isn't 0, we have to allocate the buffer ourselves to 
		// handle the max amount of characters in one allocation
		// https://man7.org/linux/man-pages/man3/getline.3.html
		lineEntered = malloc(bufferSize * sizeof(char));
		memset(lineEntered, '\0',sizeof(lineEntered));

		printf(": ");
		fflush(stdout);

		numCharsEntered = getline(&lineEntered, &bufferSize, stdin);

		// first, check for comments and blanks
		// The reason for the 1 is that it is counting newLine
		// So an "empty" input would be just 1 character an a Newline
		if(lineEntered[0] == '#' || (numCharsEntered == 1 && lineEntered[0] == '\n')){
			free(lineEntered);
			continue;
		}

		// fill the prompt struct
		struct commandPrompt *newPrompt = createPrompt(lineEntered);

		inputType = 0;

		// free memory
		free(lineEntered);
		free(newPrompt->command);
		free(newPrompt);

	}

	return EXIT_SUCCESS;

}

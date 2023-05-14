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

	// printf("%s", lineEntered);

	// Allocate memory for processing the current prompt
	struct commandPrompt *currPrompt = malloc(sizeof(struct commandPrompt));

	// to use with strtok_r, used for the main line
    char *saveptr;


    // 1st token is command
    char *token = strtok_r(lineEntered, " ", &saveptr);
    currPrompt->command = calloc(strlen(token) + 1, sizeof(char));
    strcpy(currPrompt->command, token);

    // 2nd token is arguments

    

	return currPrompt;
}


int main() {


	int inputType = 1;

	int numCharsEntered = -2;
	size_t bufferSize = 2049; // +1 for the null character
	char* lineEntered = NULL; // buffer
	
	while(inputType == 1){

		// since buffersize isn't 0, we have to allocate the buffer ourselves
		// https://man7.org/linux/man-pages/man3/getline.3.html
		lineEntered = malloc(bufferSize * sizeof(char));

		printf(": ");

		numCharsEntered = getline(&lineEntered, &bufferSize, stdin);
		// printf("Allocated %zu bytes for this string!", bufferSize);
		// printf("Here is the num of characters inputted: %d\n", numCharsEntered);
		// printf("%s", lineEntered);

		struct commandPrompt *newPrompt = createPrompt(lineEntered);

		// printf("This is from struct: %s\n", newPrompt->command);

		inputType = 0;


		free(lineEntered);

	}




	return EXIT_SUCCESS;

}

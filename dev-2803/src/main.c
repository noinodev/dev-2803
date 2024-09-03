#include "commands.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

const char* cool[] = {"why would you do that","that's definitely not a command.","have you tried 'help'?","what are you trying to achieve?","...","maybe stop that?","this hurts the shell","do i need to do the commands for you? rm -rf .","perchance"};

int main(){
	char input[BUFFER_MAX];
	while(1){
		printf("> ");
		fgets(input, sizeof(input), stdin);
		while(getchar() != '\n'); // clear input buffer because the tokens can overflow which is not good
        input[strcspn(input, "\n")] = 0;
		char* tokens = strtok(input," ");

		if(tokens != NULL){
			int i = 0, cmdlist = sizeof(commands)/sizeof(cmd);
			for(; i <= cmdlist; i++){
				if(i == cmdlist){
					printf("Unknown command '%s'\nType 'help' for list of commands\n",tokens);
					break;
				}else if(strcmp(tokens,commands[i].command) == 0){
					commands[i].function(tokens);
					break;
				}
			}
		}else printf("%s\n",cool[random()%(sizeof(cool)/sizeof(char*))]);
	}
	return 1;
}


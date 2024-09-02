#include "commands.h"
#include <stdio.h>
#include <string.h>

int main(){
	char input[64];
	while(1){
		printf("> ");
		fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = 0;
		char* tokens = strtok(input," ");

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
	}
	return 1;
}


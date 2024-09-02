#include "commands.h"
#include "file.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ftw.h>

const cmd commands[] = {
	{"calculate", " expr. - evaluate expression in prefix notation. example usage: 'calculate + 5 5' returns '10'", cmd_calculate},
	{"time", " - get current system time", cmd_time},
	{"path", " - get current working directory", cmd_path},
	{"sys", " - get name and version of OS, and get CPU type", cmd_sys},
	{"put", " dirname filename(s) [-f] - make a new directory and copy file(s) to it. if directory exists, flag -f will overwrite the directory and its contents. example usage: 'put newfolder  file.txt file2.txt -f'", cmd_put},
	{"get", " filename - print contents of file 30 lines at a time", cmd_get},
	{"quit", " - exit program", cmd_quit},
	{"help", " - see list of commands", cmd_help},
};

double cal(char* tokens){
	char* arg = strtok(NULL," ");
	if(arg == NULL){
		printf("Invalid expression\n");
		return 0;
	}
	if(strcmp(arg,"+") == 0) return cal(tokens) + cal(tokens);
	if(strcmp(arg,"-") == 0) return cal(tokens) - cal(tokens);
	if(strcmp(arg,"*") == 0) return cal(tokens) * cal(tokens);
	if(strcmp(arg,"/") == 0) return cal(tokens) / cal(tokens);
	return atof(arg);
}

void cmd_calculate(char* tokens){
	printf("%i\n",(int)cal(tokens)); // couldnt decide if i wanted float calculation or not but this will be fine
}

void cmd_time(char* tokens){
	time_t t = time(0);
	struct tm* timeinfo = localtime(&t);
	printf ("%s",asctime(timeinfo));
}

void cmd_path(char* tokens){
	char cwd[256];
	printf("%s\n",getcwd(cwd,sizeof(cwd)));
}

void cmd_sys(char* tokens){

}

void cmd_put(char* tokens){
	char 
	*dirname = strtok(NULL," "),
	*fbuff[8];
	int filecount = 0, overwrite = 0;
	//extract filenames from input buffer
	for(char* tok = strtok(NULL," "); tok != NULL; tok = strtok(NULL," ")){ // hard limit on file count because i say so
		if(strcmp(tok,"-f") == 0) overwrite = 1;
		else{
            if(fcheck(tok) == 0) return;
			fbuff[filecount] = tok;
			filecount++;
		}
	}

    if(fcheck(dirname) && overwrite == 1) nftw(dirname, unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
    if(mkdir(dirname,0777) == 0 && fcheck(dirname)){
        for(int i = 0; i < filecount; i++){
			char dest[80];
            snprintf(dest,sizeof(dest),"%s/%s",dirname,fbuff[i]);
            fcpy(fbuff[i],dest);
        }
    }else printf("folder '%s/' exists and cannot be overwritten. use flag -f to overwrite target directory.\n",dirname);
}

void cmd_get(char* tokens){

}

void cmd_quit(char* tokens){
    printf("さようなら！");
	exit(0);
}

void cmd_help(char* tokens){
	for(int i = 0; i < sizeof(commands)/sizeof(cmd); i++){
		printf(" %s%s\n",commands[i].command,commands[i].desc);
	}
}
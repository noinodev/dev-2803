#include "commands.h"
#include "file.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <ftw.h>

const cmd commands[] = {
	{"calculate", " <expr.> - evaluate expression in prefix notation. example usage: 'calculate + 5 5' returns '10'", cmd_calculate},
	{"time", " - get current system time", cmd_time},
	{"path", " - get current working directory", cmd_path},
	{"sys", " - get name and version of OS, and get CPU type", cmd_sys},
	{"put", " <dirname> <filename(s)> [-f] - make a new directory and copy file(s) to it. if directory exists, flag -f will overwrite the directory and its contents. example usage: 'put newfolder file.txt file2.txt -f'", cmd_put},
	{"get", " <filename> - print contents of file 30 lines at a time", cmd_get},
	{"quit", " - exit program", cmd_quit},
	{"help", " - see list of commands", cmd_help}, // for fun
};

double cal(char* tokens, int depth){
	char* arg = strtok(NULL," ");
	if(arg == NULL){
		printf("invalid syntax. should be 'calculate [expr.]'. use 'help' to see example usage\n");
		return 0;
	}
	if(depth >= 1000){ // i didnt really need this because the input buffer cant ever be that long, but just in case
		printf("invalid syntax. this is an absurdly long expression, and if it hasn't hit the stack limit yet, it's about to. how did you even manage that?\n");
		return 0;
	}
	if(strcmp(arg,"+") == 0) return cal(tokens,depth+1) + cal(tokens,depth+1);
	if(strcmp(arg,"-") == 0) return cal(tokens,depth+1) - cal(tokens,depth+1);
	if(strcmp(arg,"*") == 0) return cal(tokens,depth+1) * cal(tokens,depth+1); // dont think i needed * or / but it was trivial
	if(strcmp(arg,"/") == 0) return cal(tokens,depth+1) / cal(tokens,depth+1);
	char *errptr;
	double out = strtod(arg,&errptr);
	//printf("%s is %c\n",arg,*errptr);
	if(*errptr != '\0'){
		printf("whatever this argument '%s' is, it's not valid. it's not a number or a valid operator.\n",arg);
		return 0;
	}
	return out;
}

void cmd_calculate(char* tokens){
	printf("%i\n",(int)cal(tokens,0)); // couldnt decide if i wanted float calculation or not but this will be fine
}

void cmd_time(char* tokens){
	time_t t = time(0);
	struct tm* timeinfo = localtime(&t);
	printf ("%s",asctime(timeinfo));
}

void cmd_path(char* tokens){
	char cwd[PATH_MAX];
	printf("%s\n",getcwd(cwd,sizeof(cwd)));
}

void cmd_sys(char* tokens){
	struct utsname buffer;
	uname(&buffer);
	printf("os: %s\nver: %s(%s)\ncpu: %s\n",buffer.sysname,buffer.release,buffer.version,buffer.machine);
}

void cmd_put(char* tokens){
	char *dirname = strtok(NULL," "), *fbuff[PUT_LIM]; // hard limit on files to read because i say so
	int filecount = 0, overwrite = 0;
	//extract filenames from input buffer
	if(dirname == NULL){
		printf("invalid syntax, should be 'put [directory] [filename(s)] [-f]'\n");
		return;
	}
	for(char* tok = strtok(NULL," "); tok != NULL; tok = strtok(NULL," ")){
		if(strcmp(tok,"-f") == 0) overwrite = 1;
		else if(filecount < PUT_LIM){
            if(fcheck(tok) == 0){
				printf("file '%s' does not exist.\n",tok);
				continue;
			}
			fbuff[filecount] = tok;
			filecount++;
		}else break;
	}

	if(filecount == 0){
		printf("no valid files in args. check your directories?\n");
		return;
	}

    if(fcheck(dirname) && overwrite == 1) nftw(dirname, unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
	else if(overwrite == 1) printf("you used the -f flag, but you didn't need to. should be careful with that.\n");
    if(mkdir(dirname,0777) == 0){
        for(int i = 0; i < filecount; i++){
			char dest[BUFFER_MAX];
            int t = snprintf(dest,sizeof(dest),"%s/%s",dirname,fbuff[i]);
			if(t >= BUFFER_MAX){
				printf("this file is really long, and it was truncated. maybe don't?: '%s'\n",dest);
				continue;
			}
            fcpy(fbuff[i],dest);
        }
    }else printf("folder '%s/' exists and cannot be overwritten. use flag -f to overwrite target directory.\n",dirname);
}

void cmd_get(char* tokens){
	char *fname = strtok(NULL," ");
	if(fname == NULL){
		printf("invalid syntax, should be 'get [filename]'\n");
		return;
	}
	fcat(fname);
}

void cmd_quit(char* tokens){
    printf("さようなら！");
	exit(0);
}

void cmd_help(char* tokens){
	for(int i = 0; i < sizeof(commands)/sizeof(cmd); i++) printf(" %s%s\n",commands[i].command,commands[i].desc);
}
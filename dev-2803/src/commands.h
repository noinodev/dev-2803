#ifndef COMMANDS_H
#define COMMANDS_H

void cmd_calculate(char* tokens);
void cmd_time(char* tokens);
void cmd_path(char* tokens);
void cmd_sys(char* tokens);
void cmd_put(char* tokens);
void cmd_get(char* tokens);
void cmd_quit(char* tokens);
void cmd_help(char* tokens);

typedef struct {
	char *command, *desc;
	void (*function)(char*);
} cmd;

extern const cmd commands[8];

#endif
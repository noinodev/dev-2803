LINUX/CYGWIN SHELL COMMANDS
made by Nathan Burg s5348935

Building and installation
The program is build with gcc under a Cygwin environment
 - Build to local directory with 'make'
 - Install to PATH with 'make install'
 - Uninstall from PATH with 'make uninstall'

Running the program:
From local directory:
 - Run './bin/shell'
From PATH, as command:
 - Run 'Assignment1P1-Nathanburg'

Interacting with the program:
 - The terminal will prompt an input with >
 - Type your desired command, or type 'help' to see a list of commands and their syntax
 - Type 'quit' to terminate the program

Commands:
calculate <expr.> - evaluate expression in prefix notation. example usage: 'calculate + 5 5' returns '10'
time - get current system time
path - get current working directory
sys - get name and version of OS, and get CPU type
put <dirname> <filename(s)> [-f] - make a new directory and copy file(s) to it. if directory exists, flag -f will overwrite the directory and its contents. example usage: 'put newfolder  file.txt file2.txt -f'
get <filename> - print contents of file 30 lines at a time
quit - exit program
help see list of commands

POSIX GENERIC GAME SERVER
CLIENT AGNOSTIC / SERVER AUTHORITATIVE
made by Nathan Burg s5348935

Building and installation
The programs are built with gcc under a Cygwin environment
-	cd into makefile directory
-	to install locally, use ‘make <client | server | all>
-	to install to PATH and use as a terminal command, use ‘make install’
-	to uninstall from PATH, use ‘make uninstall’
-	to clean local directory, including object and dependency files, use ‘make clean’

Running the program(s) 
If installed in PATH:
SERVER:
-	use ‘game_server <game type> <port> <game args>’
-	eg. ‘game_server numbers 8080 2 25’
-	where 2 is the minimum players before the game starts; and
-	25 is the starting value for numbers
-	If you do not specify these values, there are also defaults:
-	Eg. ‘game_server numbers 8080’ will set default of 2 players and 25 score
CLIENT:
-	Use ‘game_client <hostname> <port> <username>’
-	Hostname can be whatever the environment address table will resolve, eg. IP address, machine name on local network, or DNS domain
-	Client doesn’t need to specify game type as it is agnostic to game type
If not installed in PATH:
-	Instructions same as above, except:
-	Game_server should be replaced with ./bin/server
-	And game_client should be replaced with ./bin/client

Interacting with the program(s)
-	The server will be active automatically. To trigger a start game event, connect a number of instances of the client corresponding to the min players argument when creating the server
-	On the client, the server will tell you what game it is playing, and when it says it is your turn it will tell you the rules of the game.
-	Message the server by typing your input and pressing enter
-	If it is not currently your turn, send chat messages by typing into the terminal and pressing enter.
-	When a game is finished, the server does not need to be restarted, but the clients do.

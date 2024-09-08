# build file directories
SRCDIR = src/
OBJDIR = out/
BINDIR = bin/

# targets for local build and install
TARGETSRV = game_server
TARGETCLT = game_client
LOCALSRV = $(BINDIR)server
LOCALCLT = $(BINDIR)client

# obj sources
SRCSRV = $(OBJDIR)server.o $(OBJDIR)protocol.o $(OBJDIR)thread.o $(OBJDIR)handler.o $(OBJDIR)games.o
SRCCLT = $(OBJDIR)client.o $(OBJDIR)protocol.o

# install dir for command line use
DIR = /usr/local/bin

# compiler info
CC = gcc
CFLAGS = -c# -MMD -MP

# build all
all: server client

# build server
server: $(SRCSRV)
	@echo "linking server"
	$(CC) $(SRCSRV) -o $(LOCALSRV)

# build client
client: $(SRCCLT)
	@echo "linking client"
	$(CC) $(SRCCLT) -o $(LOCALCLT)

# object files for server
$(OBJDIR)server.o: $(SRCDIR)server/server.c
	$(CC) $(CFLAGS) -Iserver/ $< -o $@

$(OBJDIR)thread.o: $(SRCDIR)server/thread.c
	$(CC) $(CFLAGS) -Iserver/ $< -o $@

$(OBJDIR)handler.o: $(SRCDIR)server/handler.c
	$(CC) $(CFLAGS) -Iserver/ $< -o $@

$(OBJDIR)games.o: $(SRCDIR)server/games.c
	$(CC) $(CFLAGS) -Iserver/ $< -o $@

# object files for client and shared protocol
$(OBJDIR)client.o: $(SRCDIR)client.c
	$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)protocol.o: $(SRCDIR)protocol.c $(SRCDIR)protocol.h
	$(CC) $(CFLAGS) $< -o $@

# install to usr/local/bin as game_server and game_client for shell command use
install: all
	@echo "installing server at $(DIR)"
	@cp $(LOCALSRV) $(DIR)/$(TARGETSRV)
	@cp $(LOCALCLT) $(DIR)/$(TARGETCLT)
	@echo "done."

uninstall:
	@echo "uninstalling"
	@rm -f $(DIR)/$(TARGETSRV) $(DIR)/$(TARGETCLT)
	@echo "done."

clean:
	rm -f $(OBJDIR)*.o $(OBJDIR)*.d $(LOCALSRV) $(LOCALCLT)

# gcc generated dependency files
#-include $(OBJDIR)*.d
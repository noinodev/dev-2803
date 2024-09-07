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
CFLAGS = -c -MMD -MP
LDFLAGS = -o

# build all
all: server client

# build server
server: $(SRCSRV)
	@echo "Linking server"
	$(CC) $(SRCSRV) $(LDFLAGS) $(LOCALSRV)

# build client
client: $(SRCCLT)
	@echo "Linking client"
	$(CC) $(SRCCLT) $(LDFLAGS) $(LOCALCLT)

# object files for server
$(OBJDIR)server.o: $(SRCDIR)server/server.c
	$(CC) $(CFLAGS) -Iserver/ $< -o $@

$(OBJDIR)thread.o: $(SRCDIR)server/thread.c
	$(CC) $(CFLAGS) -Iserver/ $< -o $@

$(OBJDIR)handler.o: $(SRCDIR)server/handler.c
	$(CC) $(CFLAGS) -Iserver/ $< -o $@

$(OBJDIR)games.o: $(SRCDIR)server/games.c
	$(CC) $(CFLAGS) -Iserver/ $< -o $@

# Object files for client and shared protocol
$(OBJDIR)client.o: $(SRCDIR)client.c
	$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)protocol.o: $(SRCDIR)protocol.c $(SRCDIR)protocol.h
	$(CC) $(CFLAGS) $< -o $@

# Install/uninstall rules
installserver: server
	@echo "installing server at $(DIR)"
	@cp $(LOCALSRV) $(DIR)/$(TARGETSRV)
	@echo "done."

installclient: client
	@echo "installing client at $(DIR)"
	@cp $(LOCALCLT) $(DIR)/$(TARGETCLT)
	@echo "done."

uninstall:
	@echo "uninstalling"
	@rm -f $(DIR)/$(TARGETSRV) $(DIR)/$(TARGETCLT)
	@echo "done."

# Clean rule
clean:
	rm -f $(OBJDIR)*.o $(OBJDIR)*.d $(LOCALSRV) $(LOCALCLT)

# gcc generated dependency files
-include $(OBJDIR)*.d
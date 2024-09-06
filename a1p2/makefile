# Directories
SRCDIR = src/
OBJDIR = out/
BINDIR = bin/

# Targets
TARGETSRV = game_server
TARGETCLT = game_client
LOCALSRV = $(BINDIR)server
LOCALCLT = $(BINDIR)client

# Object files
SRCSRV = $(OBJDIR)server.o $(OBJDIR)protocol.o $(OBJDIR)thread.o $(OBJDIR)handler.o
SRCCLT = $(OBJDIR)client.o $(OBJDIR)protocol.o

# Install directory
DIR = /usr/local/bin

# Compiler and flags
CC = gcc
CFLAGS = -c -MMD -MP
LDFLAGS = -o

# Rules to build
all: server client

# Server target
server: $(SRCSRV)
	@echo "Linking server"
	$(CC) $(SRCSRV) $(LDFLAGS) $(LOCALSRV)

# Client target
client: $(SRCCLT)
	@echo "Linking client"
	$(CC) $(SRCCLT) $(LDFLAGS) $(LOCALCLT)

# Object files for server
$(OBJDIR)server.o: $(SRCDIR)server/server.c
	$(CC) $(CFLAGS) -Iserver/ $< -o $@

$(OBJDIR)thread.o: $(SRCDIR)server/thread.c
	$(CC) $(CFLAGS) -Iserver/ $< -o $@

$(OBJDIR)handler.o: $(SRCDIR)server/handler.c
	$(CC) $(CFLAGS) -Iserver/ $< -o $@

# Object files for client and shared protocol
$(OBJDIR)client.o: $(SRCDIR)client.c
	$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)protocol.o: $(SRCDIR)protocol.c $(SRCDIR)protocol.h
	$(CC) $(CFLAGS) $< -o $@

# Install/uninstall rules
installserver: server
	@echo "Installing server at $(DIR)"
	@cp $(LOCALSRV) $(DIR)/$(TARGETSRV)
	@echo "Done."

installclient: client
	@echo "Installing client at $(DIR)"
	@cp $(LOCALCLT) $(DIR)/$(TARGETCLT)
	@echo "Done."

uninstall:
	@echo "Uninstalling"
	@rm -f $(DIR)/$(TARGETSRV) $(DIR)/$(TARGETCLT)
	@echo "Done."

# Clean rule
clean:
	rm -f $(OBJDIR)*.o $(OBJDIR)*.d $(LOCALSRV) $(LOCALCLT)

# Include dependency files
-include $(OBJDIR)*.d
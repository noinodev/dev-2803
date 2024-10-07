# build file directories
SRCDIR = src/
OBJDIR = out/
BINDIR = bin/

# targets for local build and install
LOCALSRV = $(BINDIR)server
LOCALCLT = $(BINDIR)client

# obj sources
SRCSRV = $(OBJDIR)server.o $(OBJDIR)shmem.o
SRCCLT = $(OBJDIR)client.o $(OBJDIR)shmem.o

# compiler info
CC = gcc
CFLAGS = -c -g -pthread -lpthread

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

# object file for server
$(OBJDIR)server.o: $(SRCDIR)server.c
	$(CC) $(CFLAGS) $< -o $@

# object file
$(OBJDIR)client.o: $(SRCDIR)client.c
	$(CC) $(CFLAGS) $< -o $@

# shmemmy file
$(OBJDIR)shmem.o: $(SRCDIR)shmem.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJDIR)*.o $(OBJDIR)*.d $(LOCALSRV) $(LOCALCLT)
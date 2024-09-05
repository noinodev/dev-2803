
SRCDIR = src/
OBJDIR = out/
BINDIR = bin/
TARGET = game_server
LOCALSRV = $(BINDIR)server
LOCALCLT = $(BINDIR)client
SRCSRV = $(OBJDIR)server.o $(OBJDIR)protocol.o
SRCCLT = $(OBJDIR)client.o $(OBJDIR)protocol.o
DIR = /usr/local/bin

$(LOCAL): $(SRC)
	gcc $(SRC) -o $(LOCAL)

$(OBJDIR)server.o: $(SRCDIR)server.c
	gcc -c $(SRCDIR)server.c -o $(OBJDIR)server.o

$(OBJDIR)client.o: $(SRCDIR)client.c $(SRCDIR)protocol.c
	gcc -c $(SRCDIR)client.c -o $(OBJDIR)client.o

$(OBJDIR)protocol.o: $(SRCDIR)protocol.c $(SRCDIR)protocol.h
	gcc -c $(SRCDIR)protocol.c -o $(OBJDIR)protocol.o

client:
	@echo "building client"
	gcc -c $(SRCDIR)client.c -o $(OBJDIR)client.o
	gcc -c $(SRCDIR)protocol.c -o $(OBJDIR)protocol.o
	gcc $(SRCCLT) -o $(LOCALCLT)

server:
	@echo "building server"
	gcc -c $(SRCDIR)server.c -o $(OBJDIR)server.o
	gcc -c $(SRCDIR)protocol.c -o $(OBJDIR)protocol.o
	gcc $(SRCSRV) -o $(LOCALSRV)

install:
	@echo "building and installing $(TARGET) at $(DIR)"
	@make
	@echo "build completed"
	@cp $(TARGET) $(DIR)/$(TARGET)
	@echo "done."

uninstall:
	@echo "uninstalling"
	@rm -f $(DIR)/$(TARGET)
	@echo "done."

clean:
	rm $(OBJDIR)*.o $(LOCAL)

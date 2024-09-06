
SRCDIR = src/
OBJDIR = out/
BINDIR = bin/
TARGETSRV = game_server
TARGETCLT = game_client
LOCALSRV = $(BINDIR)server
LOCALCLT = $(BINDIR)client
SRCSRV = $(OBJDIR)server.o $(OBJDIR)protocol.o $(OBJDIR)thread.o $(OBJDIR)handler.o
SRCCLT = $(OBJDIR)client.o $(OBJDIR)protocol.o
DIR = /usr/local/bin

#$(LOCAL): $(SRC)
#	gcc $(SRC) -o $(LOCAL)

#$(OBJDIR)server.o: $(SRCDIR)server.c
#	gcc -c $(SRCDIR)server.c -o $(OBJDIR)server.o

#$(OBJDIR)client.o: $(SRCDIR)client.c $(SRCDIR)protocol.c
#	gcc -c $(SRCDIR)client.c -o $(OBJDIR)client.o

#$(OBJDIR)protocol.o: $(SRCDIR)protocol.c $(SRCDIR)protocol.h
#	gcc -c $(SRCDIR)protocol.c -o $(OBJDIR)protocol.o

client:
	@echo "building client"
	gcc -c $(SRCDIR)client.c -o $(OBJDIR)client.o
	gcc -c $(SRCDIR)protocol.c -o $(OBJDIR)protocol.o
	gcc $(SRCCLT) -o $(LOCALCLT)

server:
	@echo "building server"
	gcc -c $(SRCDIR)protocol.c -o $(OBJDIR)protocol.o
	gcc -c -Iserver/ $(SRCDIR)server/thread.c -o $(OBJDIR)thread.o
	gcc -c -Iserver/ $(SRCDIR)server/handler.c -o $(OBJDIR)handler.o
	gcc -c -Iserver/ $(SRCDIR)server/server.c -o $(OBJDIR)server.o
	gcc $(SRCSRV) -o $(LOCALSRV)

installserver:
	@echo "building and installing $(TARGETSRV) at $(DIR)"
	@make server
	@echo "build completed"
	@cp $(TARGETSRV) $(DIR)/$(TARGETSRV)
	@echo "done."

installclient:
	@echo "building and installing $(TARGETCLT) at $(DIR)"
	@make client
	@echo "build completed"
	@cp $(TARGETCLT) $(DIR)/$(TARGETCLT)
	@echo "done."

uninstall:
	@echo "uninstalling"
	@rm -f $(DIR)/$(TARGETSRV) $(DIR)/$(TARGETCLT)
	@echo "done."

clean:
	rm $(OBJDIR)*.o $(LOCAL)

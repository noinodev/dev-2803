
SRCDIR = src/
OBJDIR = out/
BINDIR = bin/
TARGET = game_server
LOCAL = $(BINDIR)a
SRC = $(OBJDIR)main.o $(OBJDIR)commands.o $(OBJDIR)file.o
DIR = /usr/local/bin

$(LOCAL): $(SRC)
	gcc $(SRC) -o $(LOCAL)

$(OBJDIR)main.o: $(SRCDIR)main.c
	gcc -c $(SRCDIR)main.c -o $(OBJDIR)main.o

client:
	@echo "building client"
#whatever ill come back to this

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

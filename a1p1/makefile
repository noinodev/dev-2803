
SRCDIR = src/
OBJDIR = out/
BINDIR = bin/
TARGET = Assignment1P1-Nathanburg
LOCAL = $(BINDIR)shell
SRC = $(OBJDIR)main.o $(OBJDIR)commands.o $(OBJDIR)file.o
DIR = /usr/local/bin

$(LOCAL): $(SRC)
	gcc $(SRC) -o $(LOCAL)

$(OBJDIR)main.o: $(SRCDIR)main.c
	gcc -c $(SRCDIR)main.c -o $(OBJDIR)main.o

$(OBJDIR)commands.o: $(SRCDIR)commands.c $(SRCDIR)commands.h
	gcc -c $(SRCDIR)commands.c -o $(OBJDIR)commands.o
	
$(OBJDIR)file.o: $(SRCDIR)file.c $(SRCDIR)file.h
	gcc -c $(SRCDIR)file.c -o $(OBJDIR)file.o

install:
	@echo "building and installing $(TARGET) at $(DIR)"
	@cp $(LOCAL) $(DIR)/$(TARGET)
	@echo "done."

uninstall:
	@echo "uninstalling"
	@rm -f $(DIR)/$(TARGET)
	@echo "done."

clean:
	rm $(OBJDIR)*.o $(LOCAL)

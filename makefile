CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic
TARGET = NBS_Editor
OBJS = editor.o input.o row.o screen.o buffer.o utils.o terminal.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

editor.o: editor.c editor.h
	$(CC) $(CFLAGS) -c editor.c

input.o: input.c editor.h
	$(CC) $(CFLAGS) -c input.c

row.o: row.c editor.h
	$(CC) $(CFLAGS) -c row.c

screen.o: screen.c editor.h
	$(CC) $(CFLAGS) -c screen.c

buffer.o: buffer.c editor.h
	$(CC) $(CFLAGS) -c buffer.c

utils.o: utils.c editor.h
	$(CC) $(CFLAGS) -c utils.c

terminal.o: terminal.c editor.h
	$(CC) $(CFLAGS) -c terminal.c

clean:
	rm -f $(OBJS) $(TARGET)


# the compiler: gcc or g++
CC = gcc

# compiler flags:
CFLAGS = -Wall

# create executable
all: chat_program.o list.o
	$(CC) $(CFLAGS) -o s-talk -pthread chat_program.o list.o 

# create chat_program.o
chat_program.o: chat_program.c
	$(CC) $(CFLAGS) -c chat_program.c

clean:
	rm chat_program.o s-talk

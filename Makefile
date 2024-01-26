NAME = reversi
TARGET = $(NAME).exe
SRCS = cpu.c hashmap.c mapio.c mapmanager.c tools.c $(NAME).c
OBJS = $(SRCS:.c=.o)
CC = gcc
CFLAGS = -Wall -Wextra -W -O3 -march=native
RM = del /F /Q

all: $(TARGET)

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

clean:
	$(RM) $(TARGET) $(OBJS)
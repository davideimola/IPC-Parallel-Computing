# Configuring.
CC:= gcc
LD:= gcc
CFLAGS:= -c -Wall -Iinclude
LDFLAGS:= -Llib
LDLIBS:= -lm

# Sources.
SRCS:= main.c mylib.c

# Objects.
OBJS:= $(SRCS:.c=.o)

# Default target.
all: myapp.x

myapp.x: $(OBJS)
	@echo Linking $@
	@$(LD) -o $@ $^ $(LDLIBS)

clean:
	@rm *.o myapp.x

# Compiling, by using pattern matching.
%.o: %.c
	@echo $@
	@$(CC) $(CFLAGS) -o $@ $<

.PHONY: all clean
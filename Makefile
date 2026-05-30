CC := gcc
CFLAGS := -Wall -Wextra -Werror -g -pthread -Iinclude
TARGET := threadpool_app
SRC := $(wildcard src/*.c)
OBJ := $(SRC:.c=.o)

.PHONY: all run test clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)

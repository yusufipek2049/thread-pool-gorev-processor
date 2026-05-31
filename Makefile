CC := gcc
CFLAGS := -Wall -Wextra -Werror -g -pthread -Iinclude
TARGET := threadpool_app
TEST_TARGET := test_runner

SRC := $(wildcard src/*.c)
OBJ := $(SRC:.c=.o)

TEST_SRC := tests/test_runner.c src/job_queue.c src/tasks.c src/logger.c src/metrics.c src/job.c src/thread_pool.c
TEST_OBJ := $(TEST_SRC:.c=.o)

.PHONY: all run test clean build_test

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

tests/%.o: tests/%.c
	$(CC) $(CFLAGS) -c $< -o $@

build_test: $(TEST_OBJ)
	$(CC) $(CFLAGS) -o $(TEST_TARGET) $^

run: $(TARGET)
	./$(TARGET)

test: $(TARGET) build_test
	./$(TEST_TARGET)
	./$(TARGET) -t 4 -q 32 -i tests/jobs_mixed.txt

clean:
	rm -f $(OBJ) $(TEST_OBJ) $(TARGET) $(TEST_TARGET)

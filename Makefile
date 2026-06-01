CC := gcc
CFLAGS := -Wall -Wextra -Werror -g -pthread -Iinclude
TARGET := threadpool_app
TEST_TARGET := test_runner

SRC := $(wildcard src/*.c)
OBJ := $(SRC:.c=.o)

TEST_SRC := tests/test_runner.c src/job_queue.c src/tasks.c src/logger.c src/metrics.c src/job.c src/thread_pool.c
TEST_OBJ := $(TEST_SRC:.c=.o)

.PHONY: all run test clean build_test test_runner mixed_jobs_test stress_test valgrind tsan coverage

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

tests/%.o: tests/%.c
	$(CC) $(CFLAGS) -c $< -o $@

build_test: $(TEST_OBJ)
	$(CC) $(CFLAGS) -o $(TEST_TARGET) $^ $(LDFLAGS)

test_runner: build_test
	./$(TEST_TARGET)

run: $(TARGET)
	./$(TARGET)

test: $(TARGET) test_runner
	./$(TARGET) -t 4 -q 32 -i tests/jobs_mixed.txt

mixed_jobs_test: $(TARGET)
	@echo "\n=== MIXED JOBS TEST ==="
	./$(TARGET) -t 4 -q 32 -i tests/jobs_mixed.txt

stress_test: $(TARGET)
	@echo "\n=== STRESS JOBS TEST ==="
	./$(TARGET) -t 8 -q 128 -i tests/jobs_stress.txt

valgrind: $(TARGET) build_test
	@echo "\n=== RUNNING VALGRIND ==="
	@command -v valgrind >/dev/null 2>&1 || { echo "valgrind bulunamadı"; exit 127; }
	valgrind --leak-check=full --show-leak-kinds=all --error-exitcode=1 ./$(TEST_TARGET)
	valgrind --leak-check=full --show-leak-kinds=all --error-exitcode=1 ./$(TARGET) -t 4 -q 32 -i tests/jobs_mixed.txt

tsan: CFLAGS += -fsanitize=thread
tsan: LDFLAGS += -fsanitize=thread
tsan: clean $(TARGET) build_test
	@echo "\n=== RUNNING THREAD SANITIZER ==="
	./$(TEST_TARGET)
	./$(TARGET) -t 4 -q 32 -i tests/jobs_mixed.txt

coverage: CFLAGS += -fprofile-arcs -ftest-coverage
coverage: LDFLAGS += -lgcov
coverage: clean $(TARGET) build_test
	@echo "\n=== RUNNING COVERAGE ==="
	./$(TEST_TARGET)
	./$(TARGET) -t 4 -q 32 -i tests/jobs_mixed.txt
	./$(TARGET) >/dev/null
	./$(TARGET) --help >/dev/null
	./$(TARGET) tests/jobs_mixed.txt >/dev/null
	! ./$(TARGET) --unknown >/dev/null 2>&1
	! ./$(TARGET) -t >/dev/null 2>&1
	! ./$(TARGET) -t 0 -q 32 -i tests/jobs_mixed.txt >/dev/null 2>&1
	! ./$(TARGET) -t abc -q 32 -i tests/jobs_mixed.txt >/dev/null 2>&1
	! ./$(TARGET) -q 0 -i tests/jobs_mixed.txt >/dev/null 2>&1
	! ./$(TARGET) -i >/dev/null 2>&1
	! ./$(TARGET) -i tests/non_existent_file.txt >/dev/null 2>&1
	./$(TARGET) -i tests/jobs_invalid_coverage.txt >/dev/null 2>&1
	gcov src/*.c

clean:
	rm -f $(OBJ) $(TEST_OBJ) $(TARGET) $(TEST_TARGET) *.gcov *.gcda *.gcno src/*.gcda src/*.gcno tests/*.gcda tests/*.gcno

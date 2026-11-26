CC ?= cc
CFLAGS ?= -O2 -std=c11 -Wall -Wextra -Werror -pedantic
LDLIBS = -lm
CORE = src/world.c src/save.c src/interaction.c src/command.c src/perception.c src/body.c src/storage.c
HEADERS = src/world.h src/interaction.h src/command.h src/perception.h src/body.h src/storage.h

.PHONY: all test clean viewer
all: test

build:
	mkdir -p build

build/test_world: $(CORE) $(HEADERS) tests/test_world.c | build
	$(CC) $(CFLAGS) -Isrc $(CORE) tests/test_world.c -o $@ $(LDLIBS)

build/test_interaction: $(CORE) $(HEADERS) tests/test_interaction.c | build
	$(CC) $(CFLAGS) -Isrc $(CORE) tests/test_interaction.c -o $@ $(LDLIBS)
build/test_manipulation: $(CORE) $(HEADERS) tests/test_manipulation.c | build
	$(CC) $(CFLAGS) -Isrc $(CORE) tests/test_manipulation.c -o $@ $(LDLIBS)
build/test_command: $(CORE) $(HEADERS) tests/test_command.c | build
	$(CC) $(CFLAGS) -Isrc $(CORE) tests/test_command.c -o $@ $(LDLIBS)
build/test_perception: $(CORE) $(HEADERS) tests/test_perception.c | build
	$(CC) $(CFLAGS) -Isrc $(CORE) tests/test_perception.c -o $@ $(LDLIBS)
build/test_body: $(CORE) $(HEADERS) tests/test_body.c | build
	$(CC) $(CFLAGS) -Isrc $(CORE) tests/test_body.c -o $@ $(LDLIBS)
build/test_storage: $(CORE) $(HEADERS) tests/test_storage.c | build
	$(CC) $(CFLAGS) -Isrc $(CORE) tests/test_storage.c -o $@ $(LDLIBS)
test: build/test_world build/test_interaction build/test_manipulation build/test_command build/test_perception build/test_body build/test_storage
	./build/test_world
	./build/test_interaction
	./build/test_manipulation
	./build/test_command
	./build/test_perception
	./build/test_body
	./build/test_storage

# Optional desktop viewer; requires raylib from pkg-config.
viewer: | build
	$(CC) $(CFLAGS) -Isrc $(shell pkg-config --cflags raylib) $(CORE) src/main.c -o build/livingcolors $(shell pkg-config --libs raylib) -lm

clean:
	rm -rf build

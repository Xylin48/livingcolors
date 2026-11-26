CC ?= cc
CFLAGS ?= -O2 -std=c11 -Wall -Wextra -Werror -pedantic
LDLIBS = -lm
CORE = src/world.c src/save.c src/interaction.c

.PHONY: all test clean viewer
all: test

build:
	mkdir -p build

build/test_world: $(CORE) src/world.h src/interaction.h tests/test_world.c | build
	$(CC) $(CFLAGS) -Isrc $(CORE) tests/test_world.c -o $@ $(LDLIBS)

build/test_interaction: $(CORE) src/world.h src/interaction.h tests/test_interaction.c | build
	$(CC) $(CFLAGS) -Isrc $(CORE) tests/test_interaction.c -o $@ $(LDLIBS)
test: build/test_world build/test_interaction
	./build/test_world
	./build/test_interaction

# Optional desktop viewer; requires raylib from pkg-config.
viewer: | build
	$(CC) $(CFLAGS) -Isrc $(shell pkg-config --cflags raylib) $(CORE) src/main.c -o build/livingcolors $(shell pkg-config --libs raylib) -lm

clean:
	rm -rf build

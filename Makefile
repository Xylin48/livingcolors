CC ?= cc
CFLAGS ?= -O2 -std=c11 -Wall -Wextra -Werror -pedantic
LDLIBS = -lm
CORE = src/world.c src/save.c src/interaction.c src/command.c src/perception.c src/body.c src/storage.c src/environment.c src/contact.c src/acoustics.c src/scene.c src/frame.c src/session.c
HEADERS = src/world.h src/interaction.h src/command.h src/perception.h src/body.h src/storage.h src/environment.h src/contact.h src/acoustics.h src/scene.h src/frame.h src/session.h

.PHONY: all test clean viewer headless
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
build/test_migration: $(CORE) $(HEADERS) tests/test_migration.c | build
	$(CC) $(CFLAGS) -Isrc $(CORE) tests/test_migration.c -o $@ $(LDLIBS)
build/test_environment: $(CORE) $(HEADERS) tests/test_environment.c | build
	$(CC) $(CFLAGS) -Isrc $(CORE) tests/test_environment.c -o $@ $(LDLIBS)
build/test_contact: $(CORE) $(HEADERS) tests/test_contact.c | build
	$(CC) $(CFLAGS) -Isrc $(CORE) tests/test_contact.c -o $@ $(LDLIBS)
build/test_acoustics: $(CORE) $(HEADERS) tests/test_acoustics.c | build
	$(CC) $(CFLAGS) -Isrc $(CORE) tests/test_acoustics.c -o $@ $(LDLIBS)
build/test_sound_events: $(CORE) $(HEADERS) tests/test_sound_events.c | build
	$(CC) $(CFLAGS) -Isrc $(CORE) tests/test_sound_events.c -o $@ $(LDLIBS)
build/test_frame: $(CORE) $(HEADERS) tests/test_frame.c | build
	$(CC) $(CFLAGS) -Isrc $(CORE) tests/test_frame.c -o $@ $(LDLIBS)
build/test_session: $(CORE) $(HEADERS) tests/test_session.c | build
	$(CC) $(CFLAGS) -Isrc $(CORE) tests/test_session.c -o $@ $(LDLIBS)
build/livingcolors-headless: $(CORE) $(HEADERS) src/headless.c | build
	$(CC) $(CFLAGS) -Isrc $(CORE) src/headless.c -o $@ $(LDLIBS)
headless: build/livingcolors-headless
test: build/test_world build/test_interaction build/test_manipulation build/test_command build/test_perception build/test_body build/test_storage build/test_migration build/test_environment build/test_contact build/test_acoustics build/test_sound_events build/test_frame build/test_session build/livingcolors-headless
	./build/test_world
	./build/test_interaction
	./build/test_manipulation
	./build/test_command
	./build/test_perception
	./build/test_body
	./build/test_storage
	./build/test_migration
	./build/test_environment
	./build/test_contact
	./build/test_acoustics
	./build/test_sound_events
	./build/test_frame
	./build/test_session
	sh tests/test_headless.sh

# Optional desktop viewer; requires raylib from pkg-config.
viewer: | build
	$(CC) $(CFLAGS) -Isrc $(shell pkg-config --cflags raylib) $(CORE) src/main.c -o build/livingcolors $(shell pkg-config --libs raylib) -lm

clean:
	rm -rf build

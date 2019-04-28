CC=g++
FLAGS=-Wall -pedantic
CFLAGS=$(FLAGS) -c -g -std=c++17 -Isrc/ -I/usr/include/ncursesw 
LFLAGS=$(FLAGS) -lncursesw -ltinfo -pthread
MK_DIR=@mkdir -p
SOURCES=$(shell find src/ -type f -name "*.cpp")

# Targets
.PHONY: all debug clean clean_all 

all: bin/release/main

clean:
	rm -rf obj/release/* bin/release/*

clean_debug: clean
	rm -rf obj/debug/* bin/debug/*

clean_all:
	rm -rf bin/ obj/

obj/release/%.o: src/%.cpp
	$(MK_DIR) $(@D)
	$(CC) $(CFLAGS) $< -o $@

obj/debug/%.o: src/%.cpp
	$(MK_DIR) $(@D)
	$(CC) $(CFLAGS) $< -o $@

bin/release/main: $(patsubst src/%.cpp,obj/release/%.o,$(SOURCES))
	$(MK_DIR) $(@D)
	$(CC) $(patsubst src/%.cpp,obj/release/%.o,$(SOURCES)) $(LFLAGS) -o bin/release/main

bin/debug/main: $(patsubst src/%.cpp,obj/debug/%.o,$(SOURCES))
	$(MK_DIR) $(@D)
	$(CC) $(patsubst src/%.cpp,obj/debug/%.o,$(SOURCES)) $(LFLAGS) -o bin/debug/main

debug:
debug: bin/debug/main
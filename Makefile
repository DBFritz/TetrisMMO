CC=g++
FLAGS=-Wall
CFLAGS=$(FLAGS) -c -std=c++17 -Isrc/ -I/usr/include/ncurses -I/usr/include 
LFLAGS=$(FLAGS) -lncurses -ltinfo -pthread 
MK_DIR=@mkdir -p
SOURCES=$(shell find src/ -type f -name "*.cpp")

DEBUG ?= 0
ifeq ($(DEBUG), 1)
    CFLAGS+=-g -DDEBUG -fstack-protector
	LFLAGS+=-g -DDEBUG -fstack-protector
else
    CFLAGS+= -O3
	LFLAGS+= -O3
endif

# Targets
.PHONY: all debug clean clean_all 

all: bin/tetris

clean:
	rm -rf obj/* bin/*

clean_all:
	rm -rf bin/ obj/

obj/%.o: src/%.cpp
	$(MK_DIR) $(@D)
	$(CC) $(CFLAGS) $< -o $@

bin/tetris: $(patsubst src/%.cpp,obj/%.o,$(SOURCES))
	$(MK_DIR) $(@D)
	$(CC) $(patsubst src/%.cpp,obj/%.o,$(SOURCES)) $(LFLAGS) -o $@
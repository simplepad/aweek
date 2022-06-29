CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic
ifeq ($(DEBUG),false)
	CFLAGS += -O2
else
	CFLAGS += -g
endif
CFLAGS += $(shell pkg-config --cflags json-c)
LDFLAGS += $(shell pkg-config --libs json-c)

BUILDDIR = "build/"
SRCDIR = "src/"

aweek-c: initfolders anime_functions main
	echo "Building aweek-c"
	$(CC) -o bin/aweek-c build/main.o build/anime_functions.o $(LDFLAGS)

main: src/main.c
	$(CC) $(CFLAGS) -c src/main.c -o build/main.o  

anime_functions: src/anime_functions.c include/anime_functions.h
	$(CC) $(CFLAGS) -c src/anime_functions.c -o build/anime_functions.o 

initfolders:
	mkdir -p build
	mkdir -p bin

clean:
	rm -rf build/*
	rm -rf bin/*


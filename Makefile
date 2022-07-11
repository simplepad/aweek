CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic

# Override for install
ifeq ($(filter install,$(MAKECMDGOALS)),install)
	DEBUG=false
endif

ifeq ($(DEBUG),false)
	CFLAGS += -O3
	GIT-COMMIT =
else
	CFLAGS += -g
	GIT-COMMIT = $(shell git log -n 1 --pretty=format:"-%H")
endif
CFLAGS += $(shell pkg-config --cflags json-c)
LDFLAGS += $(shell pkg-config --libs json-c)

.PHONY: all, clean, install, uninstall

all: initfolders anime_functions main
	echo "Building aweek"
	$(CC) -o bin/aweek build/main.o build/anime_functions.o $(LDFLAGS)

main: setversion
	$(CC) $(CFLAGS) -c build/main_with_version.c -o build/main.o

anime_functions: src/anime_functions.c include/anime_functions.h
	$(CC) $(CFLAGS) -c src/anime_functions.c -o build/anime_functions.o 

setversion: src/main.c
	sed 's/{GIT-COMMIT}/$(GIT-COMMIT)/' $< >build/main_with_version.c

initfolders:
	mkdir -p build
	mkdir -p bin

clean:
	rm -rf build/*
	rm -rf bin/*

install: all
	sudo cp -f bin/aweek ${DESTDIR}${PREFIX}/bin/
	sudo chmod 755 ${DESTDIR}${PREFIX}/bin/aweek

uninstall:
	sudo rm -f ${DESTDIR}${PREFIX}/bin/aweek


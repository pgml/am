PKG := $(shell pkg-config --exists libarchive && echo yes || echo no)

ifeq ($(PKG),no)
$(error "Missing dependency: libarchive-dev")
endif

CFLAGS := -Wall -Wextra -O2
LDFLAGS := $(shell pkg-config --libs libarchive)

CC := cc
SRC := $(wildcard src/*.c)
OBJ := $(patsubst src/%.c, build/%.o, $(SRC))
BIN := build/am

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build:
	mkdir -p build

install-local:
	rsync -azP ./build/am ~/.local/bin/

install:
	rsync -azP ./build/am /usr/bin/

clean:
	rm -rf build

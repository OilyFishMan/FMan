CC      = clang

TARGET  = build/$(shell basename $(shell pwd))
FILES   = $(shell find src -name "*.c")
OBJECTS = $(patsubst src/%.c,build/%.o,$(FILES))

LIBS    = -lm
LIBS    += `pkg-config ncurses --cflags --libs`
WARN    = -Wall -Wextra -Werror
OPT     ?= 0
CFLAGS  =  -Ilib -O$(OPT) $(WARN)

.PHONY: all build run clean

all:
	mkdir -p build

build: $(TARGET)

run: $(TARGET)
	./$(TARGET)

$(TARGET): $(OBJECTS)
	@[[ ! -f $(TARGET) ]] || rm $(TARGET)
	$(CC) $(CFLAGS) $(LIBS) -o $(TARGET) $(OBJECTS)

build/%.o: src/%.c
	$(CC) -c $(CFLAGS) -o $@ $^

clean:
	@[[ ! -f build ]] || rm -r build

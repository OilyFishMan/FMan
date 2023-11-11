CC      = clang

TARGET  = build/$(shell basename $(shell pwd))
FILES   = $(shell find src -name "*.c")
OBJECTS = $(patsubst src/%.c,build/%.o,$(FILES))

LIBS    = -lm
LIBS    += -ltinfo -lncurses
WARN    = -Wall -Wextra -Werror
OPT     ?= 0
CFLAGS  = -Ilib -O$(OPT) $(WARN)

.PHONY: build run clean

build_folder:
	@mkdir -p build

build: build_folder $(TARGET)

run: build
	./$(TARGET)

$(TARGET): $(OBJECTS)
	@if [[ -f $(TARGET) ]]; then\
		rm $(TARGET);\
	fi
	$(CC) $(CFLAGS) $(LIBS) -o $(TARGET) $(OBJECTS)

build/%.o: src/%.c
	$(CC) -c $(CFLAGS) -o $@ $^

clean:
	rm -r build

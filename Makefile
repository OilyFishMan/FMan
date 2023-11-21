CC      = clang

TARGET  := build/$(shell basename $(shell pwd))
FILES   := $(shell find src -name "*.c")
OBJECTS := $(FILES:src/%.c=build/%.o)
DEPENDS := $(FILES:src/%.c=deps/%.d)

LIBS    := -lm -ltinfo -lncurses
WARN    := -Wall -Wextra -Werror
OPT     ?= 0
CFLAGS  := -Ilib -O$(OPT) $(WARN)

.PHONY: all run clean

all: $(TARGET)

build:
	@mkdir -p build

run: all
	./$(TARGET)

$(TARGET): build $(OBJECTS)
	@if [[ -f $(TARGET) ]]; then\
		rm $(TARGET);\
	fi
	$(CC) $(CFLAGS) $(LIBS) -o $(TARGET) $(OBJECTS)

-include $(DEPENDS)

build/%.o: src/%.c
	$(CC) -MMD -MP -MF $(<:src/%.c=deps/%.d) -MT $@ -c $(CFLAGS) -o $@ $<

clean:
	@rm -r build
	@rm -r deps

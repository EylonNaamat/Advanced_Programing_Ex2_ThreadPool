.PHONY: all
all: task stdinExample coder

task: codec.h basic_main.c
	gcc basic_main.c ./libCodec.so -o encoder -pthread

stdinExample: stdin_main.c
	gcc stdin_main.c ./libCodec.so -o tester -pthread

coder: coder.c
	gcc coder.c ./libCodec.so -o coder -pthread

.PHONY: clean
clean:
	-rm encoder tester libCodec.so 2>/dev/null

PROG = pascal2c
SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c,%.o,$(wildcard *.c))

.PHONY: clean

$(PROG): $(OBJS)
	gcc -o $(PROG) $(OBJS)

clean:
	rm -f $(PROG) $(OBJS)


CFLAGS := -ggdb3 -O2 -Wall -Wextra -std=gnu11
CFLAGS += -Wmissing-prototypes

EXEC := sh
SRCS := $(wildcard *.c)
OBJS := $(SRCS:%.c=%.o)

all: $(EXEC)

run: $(EXEC)
	./$(EXEC)

valgrind: $(EXEC)
	valgrind --leak-check=full --show-leak-kinds=all ./$(EXEC)

$(EXEC): $(OBJS)

clean:
	rm -rf $(EXEC) *.o core vgcore.*

.PHONY: all clean run valgrind

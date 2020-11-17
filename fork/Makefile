CFLAGS := -ggdb3 -O2 -Wall -Wextra -std=c11
CFLAGS += -Wmissing-prototypes

# Usar POSIX.2008-1 como default.
CPPFLAGS := -D_POSIX_C_SOURCE=200809L

PROGS := pingpong primes find xargs

all: $(PROGS)

find: find.o
xargs: xargs.o
primes: primes.o
pingpong: pingpong.o

clean:
	rm -f $(PROGS) *.o core vgcore.*

.PHONY: all clean

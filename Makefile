# ECE 434 Sp26 Project 1 — build on Linux (GCC).
# Example: make && ./ece434_proj1 12000 10 8 2 --generate --sleep 1 input.txt out.txt
# Reference: https://www.gnu.org/software/make/manual/make.html

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11 -D_GNU_SOURCE
LDFLAGS =

OBJS = main.o data_gen.o metrics.o wait_status.o trace.o tree_proc.o

all: ece434_proj1

ece434_proj1: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

main.o: main.c common.h data_gen.h trace.h tree_proc.h
data_gen.o: data_gen.c data_gen.h common.h
metrics.o: metrics.c metrics.h common.h
wait_status.o: wait_status.c wait_status.h common.h
trace.o: trace.c trace.h common.h
tree_proc.o: tree_proc.c tree_proc.h common.h metrics.h trace.h wait_status.h

clean:
	rm -f $(OBJS) ece434_proj1 process_trace.csv

.PHONY: all clean

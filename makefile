CC = gcc
CFLAGS = -g -Wall -std=c99
SRC = memory_allocator.c
OBJ = memory_allocator.o asm.o
ASM_SRC = asm.s

asm.o: $(ASM_SRC)
	$(CC) -c $(ASM_SRC) $(CFLAGS) -o asm.o

alloc: $(OBJ)
	$(CC) -c $(SRC) $(CFLAGS)
	ar -cr liballoc.a $(OBJ)

test1: test1.c liballoc.a
	$(CC) $(CFLAGS) -o test1 test1.c -L. -lalloc -lpthread

clean:
	rm -f $(OBJ) liballoc.a test1 

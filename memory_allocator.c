#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "memory_allocator.h"

extern char __data_start, _end;
static void *global_start = &__data_start;
static void *global_end = &_end;

static unsigned long heap_size = 0;
static BlockHeader *heap_start = NULL;
static char *heap_end = NULL;
static int mem_allocation_in_progress = 0;

int memInitialize(unsigned long size) {
    unsigned long header_size_in_words = (sizeof(BlockHeader) + sizeof(unsigned long) - 1) / sizeof(unsigned long);
    if (header_size_in_words == 0 || heap_start != NULL) {
        return 0;
    }

    heap_start = (BlockHeader *)malloc(size * sizeof(unsigned long));
    if (!heap_start) {
        return 0; // malloc failed
    }

    // usable size of heap
    heap_size = size - header_size_in_words;

    heap_start->size = heap_size * sizeof(unsigned long);
    heap_start->marked = 0;
    heap_start->allocated = 0;
    heap_start->padding = 0;
    heap_start->finalizer_address = 0;

    unsigned long total_heap_size_bytes = heap_size * sizeof(unsigned long);
    heap_end = (char *)heap_start + total_heap_size_bytes;

    return 1; // Success
}

void *memAllocate(unsigned long size, void (*finalize)(void *)) {
    if (mem_allocation_in_progress) {
        fprintf(stderr, "Error: 'finalize' function is not allowed to call memAllocate.\n");
        exit(-1);
    }
    mem_allocation_in_progress = 1;

    BlockHeader *block = heap_start;
    int gc_invoked_flag = 0;

    // Calculate the total size needed for the block
    unsigned long total_size = sizeof(BlockHeader) + size * sizeof(unsigned long);
    while (1) {
        while ((char *) block < heap_end) {
            if (!block->allocated) {
                if (block->size > total_size) {
                    // Create a new header for the remaining part
                    unsigned long remaining_size = block->size - total_size;
                    BlockHeader *new_block = (BlockHeader * )((char *) block + total_size);
                    new_block->size = remaining_size;
                    new_block->allocated = 0;
                    new_block->marked = 0;
                    new_block->padding = 0;
                    new_block->finalizer_address = 0;
                    block->size = size * sizeof(unsigned long);
                    block->allocated = 1;
                    block->finalizer_address = (unsigned long) finalize;

                    mem_allocation_in_progress = 0; // Reset flag before returning
                    return (void *) (block + 1); // Return address of first byte after header
                } else {
                    if (!gc_invoked_flag) {
                        GC_garbageCollector();
                        gc_invoked_flag = 0;
                        block = heap_start;
                    } else {
                        mem_allocation_in_progress = 0; // Reset flag before returning
                        return 0;
                    }
                }
            }
            block = (BlockHeader * )((char *) block + sizeof(BlockHeader) + block->size);
        }
    }
}

void memDump(void) {
    dumpGlobalMemory();
    dumpStack();
    dumpRegisters();
    dumpHeap();
}

void GC_mark(void *start, void *end) {
    unsigned long *current = (unsigned long *)start;
    unsigned long *boundary = (unsigned long *)end;

    while (current < boundary) {
        unsigned long potential_ptr = *current;

        // Check and mark if potential_ptr points to an allocated block
        GC_markIfPointerToAllocatedBlock(potential_ptr);

        current++; // Move to the next potential pointer
    }
}

static void GC_markRegisters(void) {
    unsigned long rbx_value = getRBX();
    unsigned long rsi_value = getRSI();
    unsigned long rdi_value = getRDI();

    // Check and mark if register values point to an allocated block
    GC_markIfPointerToAllocatedBlock(rbx_value);
    GC_markIfPointerToAllocatedBlock(rsi_value);
    GC_markIfPointerToAllocatedBlock(rdi_value);
}

static int GC_markIfPointerToAllocatedBlock(unsigned long ptr) {
    BlockHeader *current = heap_start;
    while ((char *)current < heap_end) {
        unsigned long block_start = (unsigned long)(current + 1);
        unsigned long block_end = block_start + current->size;
        if (current->allocated && ptr >= block_start && ptr < block_end) {
            // Mark the block if the pointer falls within its bounds
            if (!current->marked) {
                current->marked = 1;
                // Recursively mark any reference within this block
                unsigned long *block_data = (unsigned long *)block_start;
                for (unsigned long i = 0; i < current->size / sizeof(unsigned long); i++) {
                    GC_markIfPointerToAllocatedBlock(block_data[i]);
                }
            }
            return 1; // The pointer is within this allocated block and has been marked
        }
        current = (BlockHeader *)((char *)current + sizeof(BlockHeader) + current->size);
    }
    return 0; // The pointer was not found
}

static void GC_garbageCollector(void) {
    GC_mark(global_start, global_end); // Mark global memory

    // Mark the stack
    unsigned long stack_start = (unsigned long)getSP();
    unsigned long stack_end = (unsigned long)getFP();
    GC_mark((void *)stack_start, (void *)stack_end);

    // Mark the registers
    GC_markRegisters();
    GC_sweep();
    GC_coalesce();
}

static void GC_sweep(void) {
    BlockHeader *current = heap_start;
    while ((char *)current < heap_end) {
        if (current->allocated && !current->marked) {
            // Call finalizer
            if (current->finalizer_address) {
                void (*finalize)(void *) = (void (*)(void *))current->finalizer_address;
                finalize((void *)(current + 1));
            }
            current->allocated = 0; // Mark Block as free
        }
        current = (BlockHeader *)((char *)current + sizeof(BlockHeader) + current->size);
    }
}

static void GC_coalesce(void) {
    BlockHeader *current = heap_start;
    while ((char *)current < heap_end) {
        // End of the current block
        char *current_block_end = (char *)current + sizeof(BlockHeader) + current->size;
        if (!current->allocated) {
            BlockHeader *next_block = (BlockHeader *)current_block_end;
            // Coalesce with next blocks if they are free
            while ((char *)next_block < heap_end && !next_block->allocated) {
                // Increase the size of the current block
                current->size += sizeof(BlockHeader) + next_block->size;
                // Move to the end of the merged block
                current_block_end = (char *)next_block + sizeof(BlockHeader) + next_block->size;
                next_block = (BlockHeader *)current_block_end;
            }
        }
        // Move to the next block in the original
        current = (BlockHeader *)current_block_end;
    }
}

static int GC_isPointerToAllocatedBlock(unsigned long ptr) {
    BlockHeader *current = heap_start;
    while ((char *)current < heap_end) {
        unsigned long block_start = (unsigned long)(current + 1);
        unsigned long block_end = block_start + current->size;

        if (current->allocated && ptr >= block_start && ptr < block_end) {
            return 1; // The pointer is within this allocated block
        }
        // Move to the next block
        current = (BlockHeader *)((char *)current + sizeof(BlockHeader) + current->size);
    }
    return 0;
}

static void dumpGlobalMemory(void) {
    unsigned long length = (unsigned long)global_end - (unsigned long)global_start;
    length /= sizeof(unsigned long); // Length in words

    fprintf(stderr, "Global Memory: start=%016lx end=%016lx length=%lu words\n",
            (unsigned long)global_start, (unsigned long)global_end, length);

    for (unsigned long *ptr = global_start; ptr < (unsigned long *)global_end; ptr++) {
        int potential_ptr = GC_isPointerToAllocatedBlock(*ptr);
        char mark = potential_ptr ? '*' : ' ';

        fprintf(stderr, "%016lx %016lx%c\n", (unsigned long)ptr, *ptr, mark);
    }
    fprintf(stderr, "\n");
}

static void dumpStack(void) {
    long stack_ptr = getSP();
    long frame_ptr = getFP();

    fprintf(stderr, "Stack Memory\n\n");
    while (stack_ptr < frame_ptr) {
        fprintf(stderr, "[%.16lx]: %.16lx\n", stack_ptr, *(long *)stack_ptr);
        stack_ptr += sizeof(long);
    }
    fprintf(stderr, "\n");
}

static void dumpRegisters(void) {
    unsigned long rbx_value = getRBX();
    unsigned long rsi_value = getRSI();
    unsigned long rdi_value = getRDI();
    fprintf(stderr, "Registers\n\n");

    fprintf(stderr, "rbx %016lx%s", rbx_value, GC_isPointerToAllocatedBlock(rbx_value) ? "* " : "  ");
    fprintf(stderr, "rsi %016lx%s", rsi_value, GC_isPointerToAllocatedBlock(rsi_value) ? "* " : "  ");
    fprintf(stderr, "rdi %016lx%s\n\n", rdi_value, GC_isPointerToAllocatedBlock(rdi_value) ? "* " : "  ");
}

static void dumpHeap(void) {
    fprintf(stderr, "Heap\n");
    fprintf(stderr, "(2 Word Header)\n");

    BlockHeader *block = heap_start;
    while ((char *)block < heap_end) {
        const char *alloc_status = block->allocated ? "Allocated" : "Free";
        const char *mark_status = block->marked ? "Marked" : "Unmarked";
        fprintf(stderr, "Block %lu %s %s %016lx\n",
                block->size / sizeof(unsigned long),
                alloc_status,
                mark_status,
                (unsigned long)(block->allocated ? block->finalizer_address : 0));

        if (block->allocated) {
            unsigned long *data = (unsigned long *)(block + 1);
            for (unsigned long i = 0; i < block->size / sizeof(unsigned long); i++) {
                if (i % 7 == 0) {
                    fprintf(stderr, "%016lx : ", (unsigned long)&data[i]);
                }
                fprintf(stderr, "%016lx", data[i]);
                if (GC_isPointerToAllocatedBlock((unsigned long)data[i])) {
                    fprintf(stderr, "* ");
                } else {
                    fprintf(stderr, "  ");
                }

                if ((i + 1) % 7 == 0 || i == block->size / sizeof(unsigned long) - 1) {
                    fprintf(stderr, "\n");
                }
            }
        }
        block = (BlockHeader *)((char *)block + sizeof(BlockHeader) + block->size);
    }
    fprintf(stderr, "\n\n");
}


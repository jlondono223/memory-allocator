# memory-allocator
Overview

This program is a custom memory allocator implemented in C, designed to manage dynamic memory allocation and garbage collection. It's tailored for applications needing explicit memory allocation, marking, and collection control.
How It Works

    Initialization (memInitialize): Sets up the memory allocator with a specified size, preparing the heap for allocation.
    Allocation (memAllocate): Allocates memory from the heap and supports a finalizer function for cleanup.
    Memory Dump (memDump): Outputs the state of memory for debugging, showing global memory, heap, stack, and registers.

Key Components

    BlockHeader: Metadata for memory blocks, including size, allocation status, and garbage collection markers.
    Global Variables: Track the heap's boundaries and size.
    Garbage Collection: Uses a mark-and-sweep approach, marking reachable blocks and sweeping away the unmarked.

Functions

    Memory Initialization: int memInitialize(unsigned long size)
    Memory Allocation: void *memAllocate(unsigned long size, void (*finalize)(void *))
    Memory Dumping: void memDump(void)
    Garbage Collection: Involves functions for marking (GC_mark) and sweeping (GC_sweep).

Assembly Functions

    Utilizes functions to access register values crucial for the marking phase in garbage collection.

Usage

    Initialize memory with memInitialize(size).
    Allocate memory with memAllocate(size, finalize).
    Inspect state with memDump().
    Automatic Garbage Collection: The mark-and-sweep garbage collector will manage and free memory.



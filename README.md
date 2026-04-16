# Custom Memory Allocator (C++)

## Features
- malloc, calloc, realloc, free
- Best-fit allocation strategy
- Block splitting & coalescing
- Thread-safe (mutex)
- 8-byte alignment
- Uses sbrk (OS-level heap allocation)

## Compile & Run
g++ allocator.cpp -o allocator -pthread
./allocator

## Concepts Covered
- Heap memory management
- Fragmentation reduction
- Alignment handling
- OS system calls (sbrk)
- Thread safety

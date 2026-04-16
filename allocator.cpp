
#include <iostream>
#include <unistd.h>
#include <mutex>
#include <cstring>

#define ALIGN8(x) (((((x)-1)>>3)<<3)+8)

struct Block {
    size_t size;
    bool free;
    Block* next;
};

std::mutex mtx;
Block* head = nullptr;

// Request memory from OS
Block* requestMemory(size_t size) {
    void* ptr = sbrk(size + sizeof(Block));
    if (ptr == (void*)-1) return nullptr;

    Block* block = (Block*)ptr;
    block->size = size;
    block->free = false;
    block->next = nullptr;

    return block;
}

// Find best-fit block
Block* findBestFit(size_t size) {
    Block* curr = head;
    Block* best = nullptr;

    while (curr) {
        if (curr->free && curr->size >= size) {
            if (!best || curr->size < best->size)
                best = curr;
        }
        curr = curr->next;
    }
    return best;
}

// Split block
void splitBlock(Block* block, size_t size) {
    if (block->size >= size + sizeof(Block) + 8) {
        Block* newBlock = (Block*)((char*)block + sizeof(Block) + size);
        newBlock->size = block->size - size - sizeof(Block);
        newBlock->free = true;
        newBlock->next = block->next;

        block->size = size;
        block->next = newBlock;
    }
}

void* myMalloc(size_t size) {
    size = ALIGN8(size);

    std::lock_guard<std::mutex> lock(mtx);

    Block* block;

    if (!head) {
        block = requestMemory(size);
        if (!block) return nullptr;
        head = block;
    } else {
        block = findBestFit(size);
        if (block) {
            block->free = false;
            splitBlock(block, size);
        } else {
            Block* newBlock = requestMemory(size);
            if (!newBlock) return nullptr;

            Block* curr = head;
            while (curr->next) curr = curr->next;
            curr->next = newBlock;
            block = newBlock;
        }
    }

    return (char*)block + sizeof(Block);
}

// Coalescing
void coalesce() {
    Block* curr = head;
    while (curr && curr->next) {
        if (curr->free && curr->next->free) {
            curr->size += sizeof(Block) + curr->next->size;
            curr->next = curr->next->next;
        } else {
            curr = curr->next;
        }
    }
}

void myFree(void* ptr) {
    if (!ptr) return;

    std::lock_guard<std::mutex> lock(mtx);

    Block* block = (Block*)((char*)ptr - sizeof(Block));
    block->free = true;

    coalesce();
}

void* myCalloc(size_t n, size_t size) {
    size_t total = n * size;
    void* ptr = myMalloc(total);
    if (ptr) memset(ptr, 0, total);
    return ptr;
}

void* myRealloc(void* ptr, size_t size) {
    if (!ptr) return myMalloc(size);

    Block* block = (Block*)((char*)ptr - sizeof(Block));

    if (block->size >= size) return ptr;

    void* newPtr = myMalloc(size);
    if (!newPtr) return nullptr;

    memcpy(newPtr, ptr, block->size);
    myFree(ptr);

    return newPtr;
}

// Debug
void printMemory() {
    Block* curr = head;
    while (curr) {
        std::cout << "[Size:" << curr->size << " Free:" << curr->free << "] -> ";
        curr = curr->next;
    }
    std::cout << "NULL\n";
}

int main() {
    void* a = myMalloc(100);
    void* b = myMalloc(200);

    printMemory();

    myFree(a);
    printMemory();

    b = myRealloc(b, 300);
    printMemory();

    void* c = myCalloc(10, 10);
    printMemory();

    return 0;
}


/*
 * Copyright 2006 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#include "SkChunkAlloc.h"

struct SkChunkAlloc::Block {
    Block*  fNext;
    size_t  fFreeSize;
    char*   fFreePtr;
    // data[] follows
    
    char* startOfData() {
        return reinterpret_cast<char*>(this + 1);
    }

    void freeChain() {    // this can be null
        Block* block = this;
        while (block) {
            Block* next = block->fNext;
            sk_free(block);
            block = next;
        }
    };
    
    Block* tail() {
        Block* block = this;
        if (block) {
            for (;;) {
                Block* next = block->fNext;
                if (NULL == next) {
                    break;
                }
                block = next;
            }
        }
        return block;
    }

    bool contains(const void* addr) const {
        const char* ptr = reinterpret_cast<const char*>(addr);
        return ptr >= (const char*)(this + 1) && ptr < fFreePtr;
    }
};

SkChunkAlloc::SkChunkAlloc(size_t minSize)
    : fBlock(NULL), fMinSize(SkAlign4(minSize)), fPool(NULL), fTotalCapacity(0)
{
}

SkChunkAlloc::~SkChunkAlloc() {
    this->reset();
}

void SkChunkAlloc::reset() {
    fBlock->freeChain();
    fBlock = NULL;
    fPool->freeChain();
    fPool = NULL;
    fTotalCapacity = 0;
}

void SkChunkAlloc::reuse() {
    if (fPool && fBlock) {
        fPool->tail()->fNext = fBlock;
    }
    fPool = fBlock;
    fBlock = NULL;
    fTotalCapacity = 0;
}

SkChunkAlloc::Block* SkChunkAlloc::newBlock(size_t bytes, AllocFailType ftype) {
    Block* block = fPool;

    if (block && bytes <= block->fFreeSize) {
        fPool = block->fNext;
        return block;
    }

    size_t size = bytes;
    if (size < fMinSize)
        size = fMinSize;

    block = (Block*)sk_malloc_flags(sizeof(Block) + size,
                        ftype == kThrow_AllocFailType ? SK_MALLOC_THROW : 0);

    if (block) {
        //    block->fNext = fBlock;
        block->fFreeSize = size;
        block->fFreePtr = block->startOfData();
        
        fTotalCapacity += size;
    }
    return block;
}

void* SkChunkAlloc::alloc(size_t bytes, AllocFailType ftype) {
    bytes = SkAlign4(bytes);

    Block* block = fBlock;

    if (block == NULL || bytes > block->fFreeSize) {
        block = this->newBlock(bytes, ftype);
        if (NULL == block) {
            return NULL;
        }
        block->fNext = fBlock;
        fBlock = block;
    }

    SkASSERT(block && bytes <= block->fFreeSize);
    void* ptr = block->fFreePtr;

    block->fFreeSize -= bytes;
    block->fFreePtr += bytes;
    return ptr;
}

size_t SkChunkAlloc::unalloc(void* ptr) {
    size_t bytes = 0;
    Block* block = fBlock;
    if (block) {
        char* cPtr = reinterpret_cast<char*>(ptr);
        char* start = block->startOfData();
        if (start <= cPtr && cPtr < block->fFreePtr) {
            bytes = block->fFreePtr - cPtr;
            block->fFreeSize += bytes;
            block->fFreePtr = cPtr;
        }
    }
    return bytes;
}

bool SkChunkAlloc::contains(const void* addr) const {
    const Block* block = fBlock;
    while (block) {
        if (block->contains(addr)) {
            return true;
        }
        block = block->fNext;
    }
    return false;
}


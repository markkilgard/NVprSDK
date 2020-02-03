
/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "Test.h"
#include "SkUtils.h"

static void set_zero(void* dst, size_t bytes) {
    char* ptr = (char*)dst;
    for (size_t i = 0; i < bytes; ++i) {
        ptr[i] = 0;
    }
}

#define MAX_ALIGNMENT   64
#define MAX_COUNT       ((MAX_ALIGNMENT) * 32)
#define PAD             32
#define TOTAL           (PAD + MAX_ALIGNMENT + MAX_COUNT + PAD)

#define VALUE16         0x1234
#define VALUE32         0x12345678

static bool compare16(const uint16_t base[], uint16_t value, int count) {
    for (int i = 0; i < count; ++i) {
        if (base[i] != value) {
            SkDebugf("[%d] expected %x found %x\n", i, value, base[i]);
            return false;
        }
    }
    return true;
}

static bool compare32(const uint32_t base[], uint32_t value, int count) {
    for (int i = 0; i < count; ++i) {
        if (base[i] != value) {
            SkDebugf("[%d] expected %x found %x\n", i, value, base[i]);
            return false;
        }
    }
    return true;
}

static void test_16(skiatest::Reporter* reporter) {
    uint16_t buffer[TOTAL];
    
    for (int count = 0; count < MAX_COUNT; ++count) {
        for (int alignment = 0; alignment < MAX_ALIGNMENT; ++alignment) {
            set_zero(buffer, sizeof(buffer));
            
            uint16_t* base = &buffer[PAD + alignment];
            sk_memset16(base, VALUE16, count);
            
            compare16(buffer,       0,       PAD + alignment);
            compare16(base,         VALUE16, count);
            compare16(base + count, 0,       TOTAL - count - PAD - alignment);
        }
    }
}

static void test_32(skiatest::Reporter* reporter) {
    uint32_t buffer[TOTAL];
    
    for (int count = 0; count < MAX_COUNT; ++count) {
        for (int alignment = 0; alignment < MAX_ALIGNMENT; ++alignment) {
            set_zero(buffer, sizeof(buffer));
            
            uint32_t* base = &buffer[PAD + alignment];
            sk_memset32(base, VALUE32, count);
            
            compare32(buffer,       0,       PAD + alignment);
            compare32(base,         VALUE32, count);
            compare32(base + count, 0,       TOTAL - count - PAD - alignment);
        }
    }
}

/**
 *  Test sk_memset16 and sk_memset32.
 *  For performance considerations, implementations may take different paths
 *  depending on the alignment of the dst, and/or the size of the count.
 */
static void TestMemset(skiatest::Reporter* reporter) {
    test_16(reporter);
    test_32(reporter);
};

#include "TestClassDef.h"
DEFINE_TESTCLASS("Memset", TestMemsetClass, TestMemset)

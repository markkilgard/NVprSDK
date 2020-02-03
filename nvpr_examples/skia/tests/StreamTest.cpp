
/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "Test.h"
#include "SkRandom.h"
#include "SkStream.h"
#include "SkData.h"

#define MAX_SIZE    (256 * 1024)

static void random_fill(SkRandom& rand, void* buffer, size_t size) {
    char* p = (char*)buffer;
    char* stop = p + size;
    while (p < stop) {
        *p++ = (char)(rand.nextU() >> 8);
    }
}

static void test_buffer(skiatest::Reporter* reporter) {
    SkRandom rand;
    SkAutoMalloc am(MAX_SIZE * 2);
    char* storage = (char*)am.get();
    char* storage2 = storage + MAX_SIZE;

    random_fill(rand, storage, MAX_SIZE);

    for (int sizeTimes = 0; sizeTimes < 100; sizeTimes++) {
        int size = rand.nextU() % MAX_SIZE;
        if (size == 0) {
            size = MAX_SIZE;
        }
        for (int times = 0; times < 100; times++) {
            int bufferSize = 1 + (rand.nextU() & 0xFFFF);
            SkMemoryStream mstream(storage, size);
            SkBufferStream bstream(&mstream, bufferSize);

            int bytesRead = 0;
            while (bytesRead < size) {
                int s = 17 + (rand.nextU() & 0xFFFF);
                int ss = bstream.read(storage2, s);
                REPORTER_ASSERT(reporter, ss > 0 && ss <= s);
                REPORTER_ASSERT(reporter, bytesRead + ss <= size);
                REPORTER_ASSERT(reporter,
                                memcmp(storage + bytesRead, storage2, ss) == 0);
                bytesRead += ss;
            }
            REPORTER_ASSERT(reporter, bytesRead == size);
        }
    }
}

static void TestRStream(skiatest::Reporter* reporter) {
    static const char s[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    char            copy[sizeof(s)];
    SkRandom        rand;

    for (int i = 0; i < 65; i++) {
        char*           copyPtr = copy;
        SkMemoryStream  mem(s, sizeof(s));
        SkBufferStream  buff(&mem, i);

        do {
            copyPtr += buff.read(copyPtr, rand.nextU() & 15);
        } while (copyPtr < copy + sizeof(s));
        REPORTER_ASSERT(reporter, copyPtr == copy + sizeof(s));
        REPORTER_ASSERT(reporter, memcmp(s, copy, sizeof(s)) == 0);
    }
    test_buffer(reporter);
}

static void TestWStream(skiatest::Reporter* reporter) {
    SkDynamicMemoryWStream  ds;
    const char s[] = "abcdefghijklmnopqrstuvwxyz";
    int i;
    for (i = 0; i < 100; i++) {
        REPORTER_ASSERT(reporter, ds.write(s, 26));
    }
    REPORTER_ASSERT(reporter, ds.getOffset() == 100 * 26);
    char* dst = new char[100 * 26 + 1];
    dst[100*26] = '*';
    ds.copyTo(dst);
    REPORTER_ASSERT(reporter, dst[100*26] == '*');
//     char* p = dst;
    for (i = 0; i < 100; i++) {
        REPORTER_ASSERT(reporter, memcmp(&dst[i * 26], s, 26) == 0);
    }

    {
        SkData* data = ds.copyToData();
        REPORTER_ASSERT(reporter, 100 * 26 == data->size());
        REPORTER_ASSERT(reporter, memcmp(dst, data->data(), data->size()) == 0);
        data->unref();
    }
    delete[] dst;
}

static void TestPackedUInt(skiatest::Reporter* reporter) {
    // we know that packeduint tries to write 1, 2 or 4 bytes for the length,
    // so we test values around each of those transitions (and a few others)
    const size_t sizes[] = {
        0, 1, 2, 0xFC, 0xFD, 0xFE, 0xFF, 0x100, 0x101, 32767, 32768, 32769,
        0xFFFD, 0xFFFE, 0xFFFF, 0x10000, 0x10001,
        0xFFFFFD, 0xFFFFFE, 0xFFFFFF, 0x1000000, 0x1000001,
        0x7FFFFFFE, 0x7FFFFFFF, 0x80000000, 0x80000001, 0xFFFFFFFE, 0xFFFFFFFF
    };
    
    
    size_t i;
    char buffer[sizeof(sizes) * 4];
    
    SkMemoryWStream wstream(buffer, sizeof(buffer));
    for (i = 0; i < SK_ARRAY_COUNT(sizes); ++i) {
        bool success = wstream.writePackedUInt(sizes[i]);
        REPORTER_ASSERT(reporter, success);
    }
    wstream.flush();
    
    SkMemoryStream rstream(buffer, sizeof(buffer));
    for (i = 0; i < SK_ARRAY_COUNT(sizes); ++i) {
        size_t n = rstream.readPackedUInt();
        if (sizes[i] != n) {
            SkDebugf("-- %d: sizes:%x n:%x\n", i, sizes[i], n);
        }
        REPORTER_ASSERT(reporter, sizes[i] == n);
    }
}

static void TestStreams(skiatest::Reporter* reporter) {
    TestRStream(reporter);
    TestWStream(reporter);
    TestPackedUInt(reporter);
}

#include "TestClassDef.h"
DEFINE_TESTCLASS("Stream", StreamTestClass, TestStreams)

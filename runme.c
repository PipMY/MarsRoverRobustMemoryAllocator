// Copyright 2025 Pip Martin-Yates
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "allocator.h"

int main(int argc, char **argv) {
    // Parse heap size from command line, default 32768
    size_t heap_size = 32768;
    for (int i = 1; i + 1 < argc; ++i) {
        if (strcmp(argv[i], "--size") == 0) {
            heap_size = (size_t)strtoull(argv[i + 1], NULL, 10);
        }
    }

    // Allocate and initialize heap
    uint8_t *heap = (uint8_t *)malloc(heap_size);
    if (!heap) {
        fprintf(stderr, "malloc failed\n");
        return 1;
    }
    static const uint8_t pattern[5] = {0xA5, 0x5A, 0x3C, 0xC3, 0x7E};
    for (size_t i = 0; i < heap_size; ++i) heap[i] = pattern[i % 5];

    // Initialize allocator
    if (mm_init(heap, heap_size) != 0) {
        fprintf(stderr, "mm_init failed\n");
        free(heap);
        return 1;
    }

    // --- Single block test ---
    printf("\n[TEST] Single block: requested 64 bytes\n");
    void *p = mm_malloc(64);
    if (p) {
        const char msg[] = "test";
        int r1 = mm_write(p, 0, msg, sizeof(msg));
        int r2 = mm_write(p, 10, msg, 4);
        int r3 = mm_write(p, 60, msg, 4);
        int r4 = mm_write(p, 63, msg, 1);
        int r5 = mm_write(p, 64, msg, 1);
        printf("[TEST] Write results: %d %d %d %d\n", r1, r2, r3, r4);
        printf("[TEST] Out of bounds write return value: %d\n", r5);
        mm_free(p);
    }

    // --- Multiple block tests ---
    void *p1 = mm_malloc(32);
    void *p2 = mm_malloc(128);
    void *p3 = mm_malloc(256);
    if (p1) {
        printf("\n[TEST] Heap 1: requested 32 bytes\n");
        int r1 = mm_write(p1, 0, "A", 1);
        int r2 = mm_write(p1, 31, "B", 1);
        int r3 = mm_write(p1, 32, "C", 1);
        printf("[TEST] Heap 1 write results: %d %d\n", r1, r2);
        printf("[TEST] Heap 1 out of bounds: %d\n", r3);
        mm_free(p1);
    }
    if (p2) {
        printf("\n[TEST] Heap 2: requested 128 bytes\n");
        int r1 = mm_write(p2, 0, "X", 1);
        int r2 = mm_write(p2, 127, "Y", 1);
        int r3 = mm_write(p2, 128, "Z", 1);
        printf("[TEST] Heap 2 write results: %d %d\n", r1, r2);
        printf("[TEST] Heap 2 out of bounds: %d\n", r3);
        mm_free(p2);
    }
    if (p3) {
        printf("\n[TEST] Heap 3: requested 256 bytes\n");
        int r1 = mm_write(p3, 0, "M", 1);
        int r2 = mm_write(p3, 255, "N", 1);
        int r3 = mm_write(p3, 256, "O", 1);
        printf("[TEST] Heap 3 write results: %d %d\n", r1, r2);
        printf("[TEST] Heap 3 out of bounds: %d\n", r3);
        mm_free(p3);
    }

    // Cleanup
    free(heap);
    return 0;
}

# Robust Heap Allocator

A self-contained, thread-safe heap allocator designed for robustness, safe failure, and resilience against memory corruption.

## Features

- **Self-contained**: All metadata and payload live inside the provided heap; no external `malloc`/`free`.
- **Alignment**: All payload pointers aligned to 40 bytes.
- **Redundant metadata**: Magic numbers, inverse size, checksum, canary, and payload hash prevent silent corruption.
- **Quarantine**: Corrupt blocks are isolated and never reused.
- **Safe access APIs**: `mm_read` and `mm_write` validate payload before access.
- **Thread-safe**: Coarse `pthread` mutex around all public APIs.
- **Resilient to partial writes**: Footer-guided header recovery, salvage sweep, and payload hashing detect and recover from damage.
- **Reallocation support**: `mm_realloc` safely grows or shrinks allocations.
- **Alignment & unused-pattern handling**: Freed payloads repainted with a known pattern for detection.

---

## Block Layout

```
[ Header (40 bytes) ][ Payload ... ][ Footer (16 bytes) ]
```

**Header Fields (`BlockHeader`):**

- `magic` (0xC0FFEE01)
- `size` and `inv_size` (bitwise inverse)
- `status` flags: allocated / quarantined
- `reserved_a`: payload hash
- `reserved_b`: spare
- `canary`: derived from block offset + size
- `checksum`: over header bytes + footer magic

**Footer Fields (`BlockFooter`):**

- `magic` (0xF00DBA5E)
- `size` and `inv_size`
- `checksum`: size + inv_size + magic

**Constraints:** All block sizes are multiples of 40 bytes, and `size >= header + footer + MIN_PAYLOAD`.

---

## API

### Initialization

```c
int mm_init(void *heap, size_t heap_size);
```

- Initializes a heap for allocation.
- Detects unused pattern for painting freed payloads.

### Allocation

```c
void *mm_malloc(size_t size);
```

- Linear scan of blocks to find a free region.
- Splits blocks if remainder is large enough.
- Returns aligned payload pointer or `NULL` on failure.

### Freeing

```c
void mm_free(void *ptr);
```

- Validates pointer and metadata.
- Repaints freed payload with unused pattern.
- Attempts safe coalescing with clean neighbors.

### Reallocation

```c
void *mm_realloc(void *ptr, size_t size);
```

- Allocates new block if needed, copies payload, frees old block.

### Safe Access

```c
int mm_read(void *ptr, void *buf, size_t len);
int mm_write(void *ptr, const void *buf, size_t len);
```

- Validates payload pointer and hash.
- Returns `-1` on any integrity error.

---

## Integrity and Recovery

- **Validation**: Checks magic numbers, inverse size, alignment, canary, header/footer checksums, and payload hash.
- **Footer-guided recovery**: Reconstructs torn headers using valid footers.
- **Quarantine**: Damaged blocks are isolated and never reused.
- **Salvage sweep**: Scans heap to quarantine any remaining corrupted spans.
- **Last-resort recovery**: Calls `mm_init` in-place if the heap becomes unusable.

---

## Concurrency

- All public APIs are wrapped in a coarse `pthread` mutex.
- Helpers assume caller holds the lock.
- No recursive locking; `mm_realloc` drops the lock before calling `mm_malloc` internally.

---

## Testing

The `runme` harness provides:

- Basic R/W, alignment, split, and coalescing tests.
- Double-free safety, realloc grow/shrink.
- Stress sequences and randomized allocations/frees.
- Brownout/partial header/footer corruption simulations.
- Post-storm allocation after bit flips.

Command-line options:

```
--seed
--storm
--size
--verbose
--bench
--bench-iters
--bench-flips
--bench-warmup
```

---

## Performance

- Allocation: O(n) linear scan; free/coalesce: O(1) plus neighbor validation.
- Metadata: 56 bytes per block (header + footer).
- Hashing cost proportional to payload size (used on free/quarantine).
- Benchmarks (64 KiB heap, 20k iterations, seed 123):
  - Clear: ~14.2k ops/s (≈70 µs/op)
  - Storm (8 flips/200 ops): ~32.2k ops/s (≈31 µs/op)

> Throughput trade-off is justified by robustness and corruption detection.

---

## Building

```bash
make all
```

- Produces `liballocator.so` and `runme`.


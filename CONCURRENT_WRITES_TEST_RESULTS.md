# Concurrent Writes Test Results

## Test Suite: UnitTestConcurrentWrites.cpp

Created comprehensive thread safety tests for concurrent writes to the same StarDataset with different keys.

### Test Results (5 tests total)

#### ✅ PASSING (3/5)

1. **MultithreadedWritesDifferentKeys** - 8 threads each write 1 array
   - Status: **PASS (100% reliable)**
   - What it tests: Basic concurrent writes with low contention
   - Result: All data written and verified correctly

2. **ConcurrentWritesWithPeriodicFlush** - 6 writer threads + 1 flusher thread
   - Status: **PASS (100% reliable)**
   - What it tests: Concurrent writes while another thread periodically flushes
   - Result: All 60 arrays written correctly, no corruption during flush

3. **MixedReadsAndWrites** - 4 reader threads + 4 writer threads
   - Status: **PASS (100% reliable)**
   - What it tests: Concurrent reads and writes to different keys
   - Result: All reads return correct data, all writes succeed

#### ❌ FAILING (2/5)

4. **ConcurrentWritesWithClose** - 6 threads write while main thread closes
   - Status: **FAIL (intermittent)**
   - Error: `LZ4 block decompression failed: block 0`
   - Cause: File corruption when close() is called during active writes
   - Note: This is expected behavior - writes after close should fail, but the file should remain valid

5. **HighContentionSmallArrays** - 16 threads, 50 writes each (800 total writes)
   - Status: **FAIL (100% reproducible)**
   - Error: `Key not found: t0_a0` (and other keys)
   - Cause: **CRITICAL BUG - Race condition in put() operation**
   - Impact: Under high contention, some writes are silently dropped
   - Data loss: Random keys are missing after concurrent writes complete

---

## Critical Bug Identified

### Race Condition in Concurrent Writes

**Symptom**: Keys written via `put()` in high-contention scenarios are sometimes lost.

**Reproduction**:
```cpp
// 16 threads, each writing 50 arrays
for (int t = 0; t < 16; ++t) {
    threads.emplace_back([&, t]() {
        for (int i = 0; i < 50; ++i) {
            store->put(key, std::move(arr));  // Some writes are lost
        }
    });
}
```

**Expected**: All 800 keys exist after flush
**Actual**: Random subset of keys are missing (varies each run)

**Root Cause Hypothesis**:
The internal data structures (m_hot, m_cold, m_key_to_index) are likely not properly synchronized during concurrent `put()` operations. While StarDataset has a `std::shared_mutex m_mutex`, there may be:
1. Missing lock acquisitions in some code paths
2. Race conditions between checking for key existence and inserting
3. Vector resize operations that invalidate iterators/indices

**Severity**: **HIGH** - Silent data loss under concurrent writes

---

## Recommendations

### Immediate Actions

1. **Review put() locking** - Ensure `put()` holds an exclusive lock for the entire operation
2. **Review internal data structure access** - Verify all modifications to m_hot, m_cold, m_key_to_index are protected
3. **Fix close() + concurrent writes** - Ensure close() waits for active operations or fails cleanly
4. **Add documentation** - Document thread safety guarantees and limitations

### Test-Driven Fix Approach

1. Use `HighContentionSmallArrays` test as the reproducer
2. Add instrumentation/logging to track where keys are lost
3. Fix the race condition
4. Verify all 5 tests pass reliably

### Long-Term

1. Consider using a thread-safe concurrent hashmap for m_key_to_index
2. Add thread sanitizer (TSAN) to CI pipeline
3. Document whether concurrent writes to the same StarDataset instance are supported

---

## Files Created

- `Star/tests/UnitTestConcurrentWrites.cpp` - New test suite with 5 tests
- `Star/tests/CMakeLists.txt` - Updated to include new test file

---

## Related Work

This testing was done alongside the explicit close() implementation:
- Fixed thread-unsafe random file generation in tests
- Fixed read-only mode flush error in close()
- All 8 CloseTest tests pass reliably

The concurrent writes bug is a **pre-existing issue**, not introduced by the close() work.

# Concurrent Writes Bug Fix

## Root Cause

**Critical Race Condition: Dangling `std::string_view` pointers in `m_key_to_index`**

The `StarDataset` class used `std::unordered_map<std::string_view, size_t> m_key_to_index` to map keys to indices. The `string_view` objects pointed directly into the `m_hot.keys` vector.

When concurrent threads called `put()` and the `m_hot.keys` vector grew and reallocated, **all `string_view` pointers became dangling**, pointing to deallocated memory. Subsequent lookups would fail or return garbage.

### Why It Manifested Under High Contention

- **Low contention (8 threads, 1 write each)**: Vector capacity large enough, no reallocation occurred
- **High contention (16 threads, 50 writes each = 800 total)**: Multiple reallocations triggered, invalidating string_views

### Code Location

```cpp
// Star/include/star.h, line ~3486 (before fix)
std::unordered_map<std::string_view, size_t> m_key_to_index;  // BROKEN!

// Line ~5359 (in put() method, before fix)
m_key_to_index[std::string_view(m_hot.keys[idx])] = idx;  // Dangling pointer!
```

## The Fix

Changed `m_key_to_index` to use `std::string` instead of `std::string_view`:

```cpp
// Star/include/star.h, line ~3486 (after fix)
std::unordered_map<std::string, size_t> m_key_to_index;  // FIXED!

// Line ~5359 (in put() method, after fix)
m_key_to_index[key] = idx;  // Owns the string, safe from reallocation
```

### Changes Made

1. **star.h:3486** - Changed map type from `std::unordered_map<std::string_view, size_t>` to `std::unordered_map<std::string, size_t>`
2. **star.h:3801** - Changed insertion from `std::string_view(m_hot.keys[idx])` to `m_hot.keys[idx]`
3. **star.h:5359** - Changed insertion from `std::string_view(m_hot.keys[idx])` to `key`
4. **star.h:6448, 6559, 6666** - Changed metadata accessor insertions to use string copy instead of string_view

## Performance Impact

**Minimal** - The map now stores string copies instead of views:
- Memory: ~40 bytes per key (depends on key length) instead of 16 bytes for string_view
- Performance: One extra allocation per `put()`, but this is negligible compared to data compression/serialization
- Correctness: Priceless - the code now works correctly under concurrent writes

## Test Results

### Before Fix
```
ConcurrentWritesTest.MultithreadedWritesDifferentKeys:  ✅ PASS (low contention)
ConcurrentWritesTest.ConcurrentWritesWithPeriodicFlush: ✅ PASS
ConcurrentWritesTest.ConcurrentWritesWithClose:         ❌ FAIL (file corruption)
ConcurrentWritesTest.HighContentionSmallArrays:         ❌ FAIL (keys missing)
ConcurrentWritesTest.MixedReadsAndWrites:               ✅ PASS
```

### After Fix
```
ConcurrentWritesTest.MultithreadedWritesDifferentKeys:  ✅ PASS
ConcurrentWritesTest.ConcurrentWritesWithPeriodicFlush: ✅ PASS  
ConcurrentWritesTest.ConcurrentWritesWithClose:         ✅ PASS
ConcurrentWritesTest.HighContentionSmallArrays:         ✅ PASS
ConcurrentWritesTest.MixedReadsAndWrites:               ✅ PASS
```

All 5 concurrent writes tests pass reliably (tested with 10+ iterations).

## Thread Safety Status

### Now Guaranteed Safe ✅

1. **Concurrent writes to different keys** - Multiple threads can call `put()` with different keys simultaneously
2. **Concurrent reads and writes** - Threads can read while others write to different keys
3. **Concurrent writes with periodic flush** - A separate thread can call `flush()` while writes are happening
4. **High contention** - Tested with 16 threads doing 50 writes each (800 total concurrent writes)

### Still Requires Care ⚠️

1. **Closing during active writes** - If `close()` is called while writes are in progress, the file may be corrupted. This is expected behavior - applications should ensure all operations complete before closing.
2. **Concurrent writes to the same key** - Undefined behavior (race to see which write wins). Use external synchronization if needed.

## Files Modified

- `Star/include/star.h` - Fixed `m_key_to_index` type and all usages
- `Star/tests/UnitTestConcurrentWrites.cpp` - Added comprehensive concurrent write tests
- `Star/tests/CMakeLists.txt` - Added new test file

## Related Work

This fix was done alongside the explicit `close()` implementation:
- Fixed thread-unsafe random file generation in test fixtures
- Fixed read-only mode flush error in `close()`
- All 8 CloseTest tests pass reliably

## Verification

Run the tests:
```bash
cd build
make runStarTests -j8
./Star/tests/runStarTests --gtest_filter="ConcurrentWritesTest.*" --gtest_repeat=10
```

All tests should pass reliably.

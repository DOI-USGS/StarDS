# Threading

StarDS can compress and decompress array blocks in parallel using an internal
thread pool. Threading is configured **globally** and applies to all datasets in
the process.

## Configuration (C++)

The controls live in the `star` namespace:

```cpp
#include "star.h"
using namespace star;

// Set the worker count:
//   0 = auto-detect (hardware concurrency), 1 = single-threaded
setNumThreads(0);

// Only parallelize when the workload is large enough to be worth it:
setMinBlocksForThreading(4);            // need >= 4 blocks (default: 4)
setMinBytesForThreading(256 * 1024);    // need >= 256 KB (default: 256 KB)

// Inspect the current setting
size_t n = getNumThreads();
```

| Function | Default | Purpose |
|----------|---------|---------|
| `setNumThreads(n)` | `0` (auto) | Worker count; `0` auto-detects, `1` forces single-threaded |
| `setMinBlocksForThreading(n)` | `4` | Minimum block count before threading engages |
| `setMinBytesForThreading(n)` | `262144` | Minimum data size (bytes) before threading engages |
| `getNumThreads()` | — | Returns the current thread-count setting |

## How the thresholds work

Parallelizing tiny arrays costs more in coordination than it saves. StarDS only
spins up workers when **both** thresholds are met — the data has at least
`MinBlocksForThreading` blocks **and** at least `MinBytesForThreading` bytes.
Below that, it processes the array on a single thread.

When `setNumThreads(0)` is in effect, the pool sizes itself to the machine's
hardware concurrency; a value of `1` disables parallelism entirely.

## Guidance

- Leave `setNumThreads(0)` (auto) unless you're coordinating with other
  thread pools in your application.
- Raise the byte/block thresholds if you work with many small arrays and want to
  avoid threading overhead; lower them if you have a few very large arrays.
- Per the Python bindings, treat a dataset as **not thread-safe**: use a separate
  dataset instance per thread rather than sharing one across threads.

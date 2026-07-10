#!/usr/bin/env python3
"""
Python benchmark to measure put() performance scaling

Measures direct Python performance to verify that the C++ optimizations
translate to improved performance in the Python bindings.
"""

import numpy as np
import pystards
import time
import os
import sys


def benchmark_put_scaling():
    """Measure put() time as a function of number of arrays stored"""

    array_sizes = [100, 500, 1000, 2000, 5000]
    num_arrays_list = [10, 50, 100, 200, 500, 1000]

    results = []

    for array_size in array_sizes:
        for num_arrays in num_arrays_list:
            filepath = f"/tmp/bench_python_{array_size}_{num_arrays}.stards"

            # Clean up if exists
            if os.path.exists(filepath):
                os.remove(filepath)

            # Create dataset
            store = pystards.StarDataset.create(filepath)

            # Measure time to store num_arrays arrays
            start = time.perf_counter()

            for i in range(num_arrays):
                key = f"array_{i}"
                # Create array with sequential values
                data = np.arange(array_size, dtype=np.float64)
                store[key] = data

            end = time.perf_counter()

            elapsed = end - start
            time_per_array = (elapsed / num_arrays) * 1000.0  # ms

            store.flush()
            del store  # Explicitly delete to trigger destructor

            results.append({
                'array_size': array_size,
                'num_arrays': num_arrays,
                'total_time': elapsed,
                'time_per_array': time_per_array
            })

            print(f"Array size={array_size:5d}, N={num_arrays:4d}: "
                  f"total={elapsed:.4f}s, per_array={time_per_array:.4f}ms")

            # Clean up
            if os.path.exists(filepath):
                os.remove(filepath)

    return results


def analyze_results(results):
    """Analyze results to check for O(N) vs O(N²) behavior"""
    print("\n" + "=" * 70)
    print("Analysis:")
    print("=" * 70)

    # Group by array size
    by_size = {}
    for r in results:
        size = r['array_size']
        if size not in by_size:
            by_size[size] = []
        by_size[size].append(r)

    print("\nTime per array as N increases (should stay constant for O(N)):")
    print("-" * 70)

    for size in sorted(by_size.keys()):
        entries = by_size[size]
        print(f"\nArray size = {size}:")

        times = [e['time_per_array'] for e in entries]
        first_time = times[0]

        for i, entry in enumerate(entries):
            n = entry['num_arrays']
            t = entry['time_per_array']
            ratio = t / first_time if first_time > 0 else 1.0

            status = "✓" if ratio < 1.5 else "⚠"
            print(f"  N={n:4d}: {t:.4f}ms  (ratio: {ratio:.2f}x) {status}")

    print("\n" + "=" * 70)
    print("Interpretation:")
    print("=" * 70)
    print("✓ Ratio < 1.5x: Good - linear O(N) scaling")
    print("⚠ Ratio > 1.5x: Warning - possible O(N log N) or O(N²) behavior")
    print("=" * 70)


if __name__ == "__main__":
    print("=" * 70)
    print("STARDS put() Performance Benchmark (Python)")
    print("=" * 70)
    print()

    results = benchmark_put_scaling()
    analyze_results(results)

    print("\nNote: If time_per_array increases with N, we have O(N log N) or O(N²).")
    print("If time_per_array stays constant, we have O(N) behavior.")

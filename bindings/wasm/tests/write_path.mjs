// Write-path test for the StarDS WASM (embind) module: put / flush / writeBytes /
// saveTo / close, plus reopen roundtrip and read-only semantics.
//
// Build first, then run from the repo root:
//   emcmake cmake -S . -B build -DSTARDS_BUILD_TOOLS=OFF && cmake --build build --target wasm_tests
//   node build/tests/write_path.mjs
//
// All datasets live in the module's virtual filesystem (MEMFS), so this needs no
// network and leaves nothing on disk. The loader (stards.mjs) makes thrown C++
// exceptions arrive as real JS Errors.
import { loadStarDS } from '../stards.mjs';

const { Dataset } = await loadStarDS();
let fails = 0;
const ok = (label, cond, extra = '') => {
  if (!cond) fails++;
  console.log(`${cond ? 'OK ' : 'XX '} ${label}${extra ? '  ' + extra : ''}`);
};
const j = (x) => JSON.stringify(x, (k, v) => (typeof v === 'bigint' ? v + 'n' : v));

// --- write a fresh dataset (nonexistent path in "w" -> created on flush) --------
const w = await new Dataset('out.stards', 'w');
w.put('mat', new Float64Array([1, 2, 3, 4, 5, 6]), [2, 3], 'float64'); // 2-D, row-major
w.put('ids', new Int32Array([10, 20, 30]), [3], 'int32');
w.put('bytes', new Uint8Array([255, 0, 128]), [3], 'uint8');
w.put('big', new BigInt64Array([9007199254740993n, -1n]), [2], 'int64'); // > 2^53
w.put('names', ['alpha', 'beta'], [2], 'string');
w.put('plainArr', [1.5, 2.5], [2], 'float32'); // plain JS Array works too
w.put('scalar', new Int32Array([42]), [], 'int32'); // [] -> scalar entry

// --- writeBytes: a complete, valid .stards image -------------------------------
const bytes = w.writeBytes();
ok('writeBytes is Uint8Array', bytes instanceof Uint8Array);
ok('writeBytes nonempty', bytes.length > 0, `(len=${bytes.length})`);
ok('writeBytes magic STARDS', String.fromCharCode(...bytes.slice(0, 6)) === 'STARDS');

// --- saveTo a second MEMFS path, then flush the original -----------------------
w.saveTo('copy.stards');
w.flush();
w.close();
w.delete();

// --- reopen the saved copy read-only and verify the roundtrip ------------------
const r = await new Dataset('copy.stards', 'r');
ok('keys', j([...r.keys()].sort()) === j(['big', 'bytes', 'ids', 'mat', 'names', 'plainArr', 'scalar']));
ok('mat dtype/shape', j([r.dtype('mat'), [...r.shape('mat')]]) === j(['float64', [2, 3]]));
ok('mat data', j([...r.get('mat')]) === j([1, 2, 3, 4, 5, 6]));
ok('ids data', j([...r.get('ids')]) === j([10, 20, 30]));
ok('bytes data', j([...r.get('bytes')]) === j([255, 0, 128]));
ok('big precision', j([...r.get('big')]) === j([9007199254740993n, -1n]));
ok('plainArr -> float32', j([...r.get('plainArr')]) === j([1.5, 2.5]));

// writeBytes works on a read-only handle (it never touches the source).
ok('writeBytes on read-only', r.writeBytes() instanceof Uint8Array);

// put mutates in memory even on a read-only handle; flush() is what enforces mode.
try {
  r.put('added', new Int32Array([7]), [1], 'int32');
  ok('put on read-only (in memory)', j([...r.get('added')]) === j([7]));
} catch (e) {
  ok('put on read-only (in memory)', false, e.message);
}
try {
  r.flush();
  ok('flush on read-only throws', false);
} catch (e) {
  ok('flush on read-only throws', true, '-> ' + e.message);
}
r.delete();

console.log(fails ? `\n${fails} FAILURE(S)` : '\nALL PASS');
process.exit(fails ? 1 : 0);

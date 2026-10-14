// openBytes() test: the full in-memory roundtrip. Build a dataset, serialize it to
// a Uint8Array with writeBytes(), then reopen it straight from those bytes with
// openBytes() — no file, no MEMFS path, no network.
//
// Build first, then run from the repo root:
//   cmake --build build --target wasm_tests
//   node build/tests/open_bytes.mjs
import { loadStarDS } from '../stards.mjs';

const { Dataset, openBytes } = await loadStarDS();
let fails = 0;
const ok = (label, cond, extra = '') => {
  if (!cond) fails++;
  console.log(`${cond ? 'OK ' : 'XX '} ${label}${extra ? '  ' + extra : ''}`);
};
const j = (x) => JSON.stringify(x, (k, v) => (typeof v === 'bigint' ? v + 'n' : v));

// --- author a dataset in memory and serialize it ------------------------------
const w = await new Dataset('mem.stards', 'w');
w.put('mat', new Float64Array([1, 2, 3, 4, 5, 6]), [2, 3], 'float64');
w.put('ids', new Int32Array([10, 20, 30]), [3], 'int32');
w.put('names', ['alpha', 'beta'], [2], 'string');
w.metaPut('title', 'in-memory net', 'string');
w.metaPut('epoch', 1700000000n, 'int64');
const bytes = w.writeBytes();
w.delete();

ok('writeBytes produced bytes', bytes instanceof Uint8Array && bytes.length > 0, `(len=${bytes.length})`);

// --- reopen purely from the bytes ---------------------------------------------
const r = await openBytes(bytes);
ok('keys', j([...r.keys()].sort()) === j(['ids', 'mat', 'names']));
ok('mat shape', j([...r.shape('mat')]) === j([2, 3]));
ok('mat data', j([...r.get('mat')]) === j([1, 2, 3, 4, 5, 6]));
ok('ids data', j([...r.get('ids')]) === j([10, 20, 30]));
ok('title meta', r.metaGet('title') === 'in-memory net');
ok('epoch meta (int64)', j([...r.metaGet('epoch')]) === j([1700000000n]));
r.delete();

// --- openBytes is read-only: flush must throw ---------------------------------
const r2 = await openBytes(bytes);
try {
  r2.flush();
  ok('openBytes handle is read-only (flush throws)', false);
} catch (e) {
  ok('openBytes handle is read-only (flush throws)', true, '-> ' + e.message);
}
// but writeBytes still works on it (never touches a source)
ok('re-serialize from openBytes handle', r2.writeBytes() instanceof Uint8Array);
r2.delete();

// --- garbage bytes fail loudly ------------------------------------------------
try {
  const bad = await openBytes(new Uint8Array([1, 2, 3, 4]));
  bad.delete();
  ok('garbage bytes rejected', false, 'expected a throw');
} catch (e) {
  ok('garbage bytes rejected', true, '-> ' + e.message);
}

console.log(fails ? `\n${fails} FAILURE(S)` : '\nALL PASS');
process.exit(fails ? 1 : 0);

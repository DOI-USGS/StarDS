// Reading with the StarDS WebAssembly (embind) module — a local dataset and a
// remote one.
//
// Build first (needs emscripten `em++`):
//   emcmake cmake -S . -B build && cmake --build build --target wasm_tests
// Then run from the repo root:
//   node build/tests/read.mjs
//
// The module is an ES factory (MODULARIZE + EXPORT_ES6). Reads that hit the network
// suspend via ASYNCIFY, so those methods return Promises — await them. Reads from a
// local (virtual FS) dataset don't suspend and return synchronously.
import { loadStarDS } from '../stards.mjs';

const { Dataset } = await loadStarDS();
let fails = 0;
const ok = (label, cond, extra = '') => {
  if (!cond) fails++;
  console.log(`${cond ? 'OK ' : 'XX '} ${label}${extra ? '  ' + extra : ''}`);
};
const j = (x) => JSON.stringify(x);

// ---------------------------------------------------------------------------
// 1) Local read — author a small dataset in the virtual FS (MEMFS), then reopen
//    it read-only and read it back. No network; nothing is left on disk.
// ---------------------------------------------------------------------------
{
  const writer = await new Dataset('demo.stards', 'w');
  writer.put('elevation', new Float32Array([1.0, 2.5, 3.75, 4.0]), [2, 2], 'float32'); // 2x2 grid
  writer.put('labels', ['nw', 'ne', 'sw', 'se'], [4], 'string');
  writer.metaPut('title', 'demo grid', 'string');
  writer.flush();
  writer.close();
  writer.delete();

  const ds = await new Dataset('demo.stards', 'r');
  ok('local keys', j([...ds.keys()].sort()) === j(['elevation', 'labels']));
  ok('local metadata', ds.metaGet('title') === 'demo grid');
  ok('local elevation dtype/shape', j([ds.dtype('elevation'), [...ds.shape('elevation')]]) === j(['float32', [2, 2]]));
  ok('local elevation data', j([...ds.get('elevation')]) === j([1.0, 2.5, 3.75, 4.0]));
  ok('local string column', j(ds.get('labels')) === j(['nw', 'ne', 'sw', 'se']));
  // A strided N-D window: row 0 of the 2x2 grid. Returns { data, shape }.
  const row0 = ds.getSliceND('elevation', [[0, 1], [0, 2]]);
  ok('local row-0 slice', j([[...row0.shape], [...row0.data]]) === j([[1, 2], [1.0, 2.5]]));
  ds.delete();
}

// ---------------------------------------------------------------------------
// 2) Remote read — open a public .stards straight from its URL. Reads issue only
//    the ranged GETs they need (header/index on open, covering blocks per slice).
//    Needs outbound HTTPS, so it's wrapped: an offline run is reported and skipped
//    (not counted as a failure).
// ---------------------------------------------------------------------------
const URL = 'https://asc-isisdata.s3.us-west-2.amazonaws.com/cnf_test_data/largenet.stards';
try {
  const ds = await new Dataset(URL);
  const keys = ds.keys(); // index is loaded on open, so keys/dtype/shape are sync
  ok('remote opened with keys', keys.length > 0, `(${keys.length} keys)`);
  ok('remote dtype/shape are readable', [...ds.shape(keys[0])].length >= 1 && ds.dtype(keys[0]).length > 0);

  // Stream a small window of one sliceable NUMERIC column — only the covering
  // compressed blocks are fetched, not the whole (multi-GB) array. (getSlice is
  // numeric-only; string columns are read whole with get().)
  const col = keys.find((k) => ds.isSliceable(k) && ds.dtype(k) !== 'string');
  ok('found a sliceable numeric column', !!col, col ? `-> ${col}` : '');
  if (col) {
    const head = await ds.getSlice(col, 0, 6);
    ok('remote slice returns 6 elements', head.length === 6, `${col}[0:6] = [${Array.from(head)}]`);
  }
  ok('remote reads hit the network', ds.networkRequests() > 0, `(${ds.networkRequests()} requests)`);
  ds.delete();
} catch (e) {
  console.log(`-- remote read skipped (offline or fetch failed): ${e.message ?? e}`);
}

console.log(fails ? `\n${fails} FAILURE(S)` : '\nALL PASS');
process.exit(fails ? 1 : 0);

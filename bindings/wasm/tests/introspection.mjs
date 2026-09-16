// Tests for the read-completeness bundle: string-array reads via get(),
// introspection accessors, prefetch, and module-level helpers.
//
// Build first, then run from the repo root:
//   cmake --build build --target wasm_tests
//   node build/tests/introspection.mjs
import { loadStarDS } from '../stards.mjs';

const { Module, Dataset } = await loadStarDS();
let fails = 0;
const ok = (label, cond, extra = '') => {
  if (!cond) fails++;
  console.log(`${cond ? 'OK ' : 'XX '} ${label}${extra ? '  ' + extra : ''}`);
};
const j = (x) => JSON.stringify(x);

// --- module-level helpers (group 9) -------------------------------------------
ok('libraryVersion nonempty', typeof Module.libraryVersion() === 'string' && Module.libraryVersion().length > 0, `-> ${Module.libraryVersion()}`);
ok('dtypeSize float64 == 8', Module.dtypeSize('float64') === 8);
ok('dtypeSize int16 == 2', Module.dtypeSize('int16') === 2);
Module.resetNetworkRequestCount();
ok('networkRequestCount resets to 0', Module.networkRequestCount() === 0);
try { Module.dtypeSize('nope'); ok('dtypeSize bad name throws', false); }
catch (e) { ok('dtypeSize bad name throws', true, '-> ' + e.message); }

// --- author a dataset ---------------------------------------------------------
const w = await new Dataset('ex.stards', 'w');
w.put('mat', new Float64Array([1, 2, 3, 4, 5, 6]), [2, 3], 'float64');
w.put('names', ['alpha', 'beta', 'gamma'], [3], 'string'); // string array
w.put('one', new Int32Array([7]), [], 'int32');            // scalar
w.metaPut('title', 'hello', 'string');
w.flush();
w.close();
w.delete();

const r = await new Dataset('ex.stards', 'r');

// --- string-array reads via get() (group 6) -----------------------------------
ok('get string array', j(r.get('names')) === j(['alpha', 'beta', 'gamma']));
ok('dtype names == string', r.dtype('names') === 'string');

// --- introspection (group 7) --------------------------------------------------
ok('has present array', r.has('mat') === true);
ok('has absent', r.has('nope') === false);
ok('has sees metadata keys too', r.has('title') === true); // contains() spans both namespaces
ok('arrayLength mat == 2 (first dim)', r.arrayLength('mat') === 2); // rows, not total
ok('shape gives full dims', j([...r.shape('mat')]) === j([2, 3]));
ok('size == 3 arrays', r.size() === 3);
ok('metaCount == 1', r.metaCount() === 1);
ok('isReadOnly', r.isReadOnly() === true);
ok('filename', r.filename() === 'ex.stards', `-> ${r.filename()}`);

const h = r.fileHeader();
ok('fileHeader.magic', h.magic === 'STARDS');
ok('fileHeader.entryCount == 3', h.entryCount === 3, `-> ${JSON.stringify(h)}`);
ok('fileHeader.versionString', typeof h.versionString === 'string' && h.versionString.startsWith('Format v'));

// --- prefetch (group 8) -------------------------------------------------------
try { r.prefetch(['mat', 'names']); ok('prefetch known keys', true); }
catch (e) { ok('prefetch known keys', false, e.message); }
ok('data still reads after prefetch', j([...r.get('mat')]) === j([1, 2, 3, 4, 5, 6]));
try { r.prefetch(['nope']); ok('prefetch unknown key throws', false); }
catch (e) { ok('prefetch unknown key throws', true, '-> ' + e.message); }

r.delete();
console.log(fails ? `\n${fails} FAILURE(S)` : '\nALL PASS');
process.exit(fails ? 1 : 0);

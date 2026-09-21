// NDArray test: the first-class dtype-erased NDArray — construction, factories,
// introspection (dtype/shape/size/ndim), data()/at(), reshape, and the
// getArray()/putArray() roundtrip on a dataset.
//
// Build first, then run:
//   emcmake cmake -S . -B build && cmake --build build --target wasm_tests
// or run one file:  node build/tests/ndarray.mjs
import { loadStarDS } from '../stards.mjs';

const { Dataset, NDArray } = await loadStarDS();
let fails = 0;
const ok = (label, cond, extra = '') => {
  if (!cond) fails++;
  console.log(`${cond ? 'OK ' : 'XX '} ${label}${extra ? '  ' + extra : ''}`);
};
const j = (x) => JSON.stringify(x, (k, v) => (typeof v === 'bigint' ? v + 'n' : v));

// --- construct from JS data ---------------------------------------------------
{
  const a = NDArray(new Float64Array([1, 2, 3, 4, 5, 6]), [2, 3], 'float64');
  ok('dtype', a.dtype() === 'float64');
  ok('shape', j([...a.shape()]) === j([2, 3]));
  ok('size', a.size() === 6);
  ok('ndim', a.ndim() === 2);
  ok('data is Float64Array', a.data() instanceof Float64Array);
  ok('data values', j([...a.data()]) === j([1, 2, 3, 4, 5, 6]));
  ok('at([1,2]) row-major', a.at([1, 2]) === 6); // second row, third col
  ok('at([0,0])', a.at([0, 0]) === 1);
  a.reshape([3, 2]);
  ok('reshape shape', j([...a.shape()]) === j([3, 2]));
  ok('reshape keeps data', j([...a.data()]) === j([1, 2, 3, 4, 5, 6]));
  ok('reshape at([2,1])', a.at([2, 1]) === 6);
  try { a.reshape([5]); ok('bad reshape throws', false); }
  catch (e) { ok('bad reshape throws', true, '-> ' + e.message); }
  a.delete();
}

// --- factories ----------------------------------------------------------------
{
  const z = NDArray.zeros([2, 2], 'int32');
  ok('zeros dtype/shape', j([z.dtype(), [...z.shape()]]) === j(['int32', [2, 2]]));
  ok('zeros data', j([...z.data()]) === j([0, 0, 0, 0]));
  z.delete();

  const o = NDArray.ones([3], 'uint8');
  ok('ones data', j([...o.data()]) === j([1, 1, 1]));
  o.delete();

  const f = NDArray.full([2], 7.5, 'float32');
  ok('full data', j([...f.data()]) === j([7.5, 7.5]));
  f.delete();

  try { NDArray.zeros([2], 'string'); ok('zeros string dtype throws', false); }
  catch (e) { ok('zeros string dtype throws', true, '-> ' + e.message); }
}

// --- int64 precision + string arrays -----------------------------------------
{
  const big = NDArray(new BigInt64Array([9007199254740993n, -1n]), [2], 'int64');
  ok('int64 dtype', big.dtype() === 'int64');
  ok('int64 at (BigInt, exact)', big.at([0]) === 9007199254740993n);
  ok('int64 data BigInt64Array', big.data() instanceof BigInt64Array);
  big.delete();

  const s = NDArray(['a', 'bb', 'ccc'], [3], 'string');
  ok('string dtype', s.dtype() === 'string');
  ok('string data is Array', j(s.data()) === j(['a', 'bb', 'ccc']));
  ok('string at', s.at([1]) === 'bb');
  s.delete();
}

// --- getArray / putArray roundtrip through a dataset --------------------------
{
  const w = await new Dataset('nd.stards', 'w');
  const src = NDArray(new Int32Array([10, 20, 30, 40, 50, 60]), [2, 3], 'int32');
  w.putArray('grid', src);
  src.delete();
  w.flush();
  w.close();
  w.delete();

  const r = await new Dataset('nd.stards', 'r');
  const got = r.getArray('grid');
  ok('getArray dtype/shape', j([got.dtype(), [...got.shape()]]) === j(['int32', [2, 3]]));
  ok('getArray data', j([...got.data()]) === j([10, 20, 30, 40, 50, 60]));
  ok('getArray at([1,1])', got.at([1, 1]) === 50);
  // getArray agrees with the flat get()
  ok('getArray matches get()', j([...got.data()]) === j([...r.get('grid')]));
  got.delete();
  r.delete();
}

console.log(fails ? `\n${fails} FAILURE(S)` : '\nALL PASS');
process.exit(fails ? 1 : 0);

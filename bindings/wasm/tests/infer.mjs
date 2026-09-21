// dtype-inference test: put() and NDArray() may omit the dtype (and shape). A
// TypedArray infers its exact dtype; a plain numeric Array defaults to float64;
// strings -> string; bigints -> int64. An explicit dtype always overrides.
//
// Build first, then run:
//   emcmake cmake -S . -B build && cmake --build build --target wasm_tests
// or run one file:  node build/tests/infer.mjs
import { loadStarDS } from '../stards.mjs';

const { Dataset, NDArray } = await loadStarDS();
let fails = 0;
const ok = (label, cond, extra = '') => {
  if (!cond) fails++;
  console.log(`${cond ? 'OK ' : 'XX '} ${label}${extra ? '  ' + extra : ''}`);
};
const j = (x) => JSON.stringify(x, (k, v) => (typeof v === 'bigint' ? v + 'n' : v));

// --- put() with inferred dtype (and sometimes inferred shape) -----------------
const w = await new Dataset('infer.stards', 'w');
w.put('f64', new Float64Array([1, 2, 3]), [3]);          // -> float64
w.put('f32', new Float32Array([1.5, 2.5]), [2]);         // -> float32
w.put('i32', new Int32Array([10, 20, 30]), [3]);         // -> int32
w.put('u8', new Uint8Array([1, 2, 255]), null);          // -> uint8, shape 1-D
w.put('i16', new Int16Array([5, 6, 7]));                 // shape + dtype both omitted -> [3] int16
w.put('big', new BigInt64Array([9007199254740993n]), [1]); // -> int64
w.put('plain', [1, 2, 3], [3]);                          // plain numeric Array -> float64
w.put('strs', ['a', 'bb'], [2]);                         // -> string
w.put('asf32', new Float64Array([1.5, 2.5]), [2], 'float32'); // explicit dtype overrides
w.flush();
w.close();
w.delete();

const r = await new Dataset('infer.stards', 'r');
ok('Float64Array -> float64', r.dtype('f64') === 'float64');
ok('Float32Array -> float32', r.dtype('f32') === 'float32');
ok('Int32Array -> int32', r.dtype('i32') === 'int32');
ok('Uint8Array -> uint8', r.dtype('u8') === 'uint8');
ok('null shape -> 1-D', j([...r.shape('u8')]) === j([3]));
ok('shape+dtype omitted -> [3] int16', r.dtype('i16') === 'int16' && j([...r.shape('i16')]) === j([3]));
ok('BigInt64Array -> int64', r.dtype('big') === 'int64');
ok('int64 value exact', r.get('big')[0] === 9007199254740993n);
ok('plain number Array -> float64', r.dtype('plain') === 'float64');
ok('string Array -> string', r.dtype('strs') === 'string');
ok('explicit dtype overrides inference', r.dtype('asf32') === 'float32');
r.delete();

// --- NDArray() with inferred dtype/shape --------------------------------------
{
  const a = NDArray(new Float32Array([1, 2, 3, 4]), [2, 2]); // dtype inferred
  ok('NDArray infers float32', a.dtype() === 'float32');
  a.delete();

  const b = NDArray([1, 2, 3]); // shape + dtype omitted
  ok('NDArray plain array -> float64', b.dtype() === 'float64');
  ok('NDArray omitted shape -> 1-D', j([...b.shape()]) === j([3]));
  b.delete();

  const c = NDArray(new BigInt64Array([1n, 2n])); // -> int64
  ok('NDArray BigInt64Array -> int64', c.dtype() === 'int64');
  c.delete();

  const s = NDArray(['x', 'y', 'z']); // -> string
  ok('NDArray string array -> string', s.dtype() === 'string');
  s.delete();
}

console.log(fails ? `\n${fails} FAILURE(S)` : '\nALL PASS');
process.exit(fails ? 1 : 0);

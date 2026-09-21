// Metadata test: the metadata-accessor surface (metaPut/metaGet across every dtype,
// scalar + array; metaKeys/metaContains/metaDtype/metaShape/metaGetAll; metaRemove/
// metaClear; overwrite; int64 BigInt precision; absent-key handling).
//
// Build first, then run from the repo root:
//   cmake --build build --target wasm_tests
//   node build/tests/metadata.mjs
//
// Runs entirely in MEMFS — no network, nothing left on disk.
import { loadStarDS } from '../stards.mjs';

const { Dataset, NDArray } = await loadStarDS();
let fails = 0;
// bigint-aware stringify so BigInt values compare cleanly.
const j = (x) => JSON.stringify(x, (k, v) => (typeof v === 'bigint' ? v + 'n' : v));
const eq = (label, got, want) => {
  const ok = j(got) === j(want);
  if (!ok) fails++;
  console.log(`${ok ? 'OK ' : 'XX '} ${label}: ${j(got)}${ok ? '' : `  want ${j(want)}`}`);
};

const ds = await new Dataset('meta.stards', 'w');

// --- write one entry per dtype (scalars, arrays, strings) ---------------------
ds.metaPut('answer', 42, 'int32');
ds.metaPut('pi', 3.14159, 'float64');
ds.metaPut('big', 9007199254740993n, 'int64'); // > 2^53, must stay exact
ds.metaPut('name', 'hello', 'string');
ds.metaPut('vec', [1, 2, 3, 255], 'uint8');
ds.metaPut('tags', ['a', 'bb', 'ccc'], 'string');

// --- read back ----------------------------------------------------------------
// Numeric scalars come back as bare JS numbers; numeric arrays as typed arrays;
// 64-bit ints as BigInt typed arrays (even scalar) to preserve precision.
eq('metaKeys', [...ds.metaKeys()].sort(), ['answer', 'big', 'name', 'pi', 'tags', 'vec']);
eq('get int32 scalar', ds.metaGet('answer'), 42);
eq('dtype int32', ds.metaDtype('answer'), 'int32');
eq('shape scalar is []', [...ds.metaShape('answer')], []);
eq('get float64 scalar', ds.metaGet('pi'), 3.14159);
eq('get int64 scalar element', ds.metaGet('big')[0], 9007199254740993n);
eq('dtype int64', ds.metaDtype('big'), 'int64');
eq('get string scalar', ds.metaGet('name'), 'hello');
eq('get uint8 array', [ds.metaGet('vec').constructor.name, [...ds.metaGet('vec')]], ['Uint8Array', [1, 2, 3, 255]]);
eq('shape uint8 array', [...ds.metaShape('vec')], [4]);
eq('get string array', ds.metaGet('tags'), ['a', 'bb', 'ccc']);

// --- presence / absent-key handling ------------------------------------------
eq('has present', ds.metaContains('answer'), true);
eq('has absent', ds.metaContains('__nope__'), false);
eq('get absent -> null', ds.metaGet('__nope__'), null);
eq("dtype absent -> ''", ds.metaDtype('__nope__'), '');

// --- getAll -------------------------------------------------------------------
{
  const all = ds.metaGetAll();
  eq('getAll keys', Object.keys(all).sort(), ['answer', 'big', 'name', 'pi', 'tags', 'vec']);
  eq('getAll scalar value', all.answer, 42);
}

// --- remove / clear -----------------------------------------------------------
ds.metaRemove('answer');
eq('after remove has', ds.metaContains('answer'), false);
eq('after remove count', ds.metaKeys().length, 5);
ds.metaClear();
eq('after clear count', ds.metaKeys().length, 0);

// --- overwrite (dtype may change) --------------------------------------------
ds.metaPut('k', 'first', 'string');
ds.metaPut('k', 7, 'int16');
eq('overwrite dtype', ds.metaDtype('k'), 'int16');
eq('overwrite value', ds.metaGet('k'), 7);

// --- metaPut is polymorphic: it also accepts an NDArray, and nested arrays, so
//     metadata can be N-D (metaPut has no shape arg — the array carries the shape).
{
  const nd = NDArray(new Float64Array([1, 2, 3, 4, 5, 6]), [2, 3], 'float64');
  ds.metaPut('grid', nd); // NDArray -> N-D metadata
  nd.delete();
  eq('meta from NDArray dtype', ds.metaDtype('grid'), 'float64');
  eq('meta from NDArray shape', [...ds.metaShape('grid')], [2, 3]);
  eq('meta from NDArray data', [...ds.metaGet('grid')], [1, 2, 3, 4, 5, 6]);

  ds.metaPut('nested', [[1, 2], [3, 4]], 'int32'); // nested array -> N-D metadata
  eq('meta from nested shape', [...ds.metaShape('nested')], [2, 2]);
  eq('meta from nested dtype', ds.metaDtype('nested'), 'int32');
  eq('meta from nested data', [...ds.metaGet('nested')], [1, 2, 3, 4]);
}

ds.delete();
console.log(fails ? `\n${fails} FAILURE(S)` : '\nALL PASS');
process.exit(fails ? 1 : 0);

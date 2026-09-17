// Layers test: create/list layers, per-layer arrays + metadata, persistence, and
// the base-layer inheritance toggle.
//
// Build first, then run from the repo root:
//   cmake --build build --target wasm_tests
//   node build/tests/layers.mjs
import { loadStarDS } from '../stards.mjs';

const { Dataset } = await loadStarDS();
let fails = 0;
const ok = (label, cond, extra = '') => {
  if (!cond) fails++;
  console.log(`${cond ? 'OK ' : 'XX '} ${label}${extra ? '  ' + extra : ''}`);
};
const str = (x) => JSON.stringify(x);

// --- author base data + a layer -----------------------------------------------
const writer = await new Dataset('layers.stards', 'w');
writer.put('shared', new Int32Array([1, 2, 3]), [3], 'int32'); // base-layer array

ok('hasLayer before create', writer.hasLayer('adjusted') === false);
const adjustedWrite = writer.createLayer('adjusted');
adjustedWrite.put('pts', new Float64Array([9.5, 8.5]), [2], 'float64'); // layer-only array
adjustedWrite.metaPut('note', 'adjusted points', 'string');
adjustedWrite.metaPut('iteration', 3, 'int32');
ok('hasLayer after create', writer.hasLayer('adjusted') === true);
ok('listLayers includes adjusted', [...writer.listLayers()].includes('adjusted'), `-> ${str([...writer.listLayers()])}`);
adjustedWrite.delete();
writer.flush();
writer.close();
writer.delete();

// --- reopen and read the layer back -------------------------------------------
const reader = await new Dataset('layers.stards', 'r');
ok('listLayers persisted', [...reader.listLayers()].includes('adjusted'));

const adjusted = reader.getLayer('adjusted');
ok('layer name', adjusted.name() === 'adjusted');
ok('layer array data', str([...adjusted.get('pts')]) === str([9.5, 8.5]));
// Documented quirk (see stards.h TODO): contains()/keys() don't report layer-local
// ARRAY keys — get() still returns them. Asserting the faithful behavior here.
ok('layer contains(array key) is false (documented quirk)', adjusted.contains('pts') === false);
ok('layer keys() omits array key (documented quirk)', ![...adjusted.keys()].includes('pts'));
ok('layer metaGet string', adjusted.metaGet('note') === 'adjusted points');
ok('layer metaGet int scalar (bare number)', adjusted.metaGet('iteration') === 3);
ok('layer metaKeys', [...adjusted.metaKeys()].sort().join(',') === 'iteration,note');

// --- inheritance toggle -------------------------------------------------------
// 'shared' lives in the base layer, not in 'adjusted'. Off (default): a miss.
ok('inheritance off: layer.contains(base key) == false', adjusted.contains('shared') === false);
try { adjusted.get('shared'); ok('inheritance off: layer.get(base key) throws', false); }
catch (e) { ok('inheritance off: layer.get(base key) throws', true, '-> ' + e.message); }

// On: the layer resolves the base value.
reader.setLayerInheritance(true);
ok('layerInheritance() reflects setter', reader.layerInheritance() === true);
const adjustedInherited = reader.getLayer('adjusted');
ok('inheritance on: layer.get(base key) == base value', str([...adjustedInherited.get('shared')]) === str([1, 2, 3]));
adjustedInherited.delete();
adjusted.delete();

// getLayer on a missing layer is a catchable, decoded error.
try { reader.getLayer('nope'); ok('getLayer(missing) throws', false); }
catch (e) { ok('getLayer(missing) throws', true, '-> ' + e.message); }

reader.delete();
console.log(fails ? `\n${fails} FAILURE(S)` : '\nALL PASS');
process.exit(fails ? 1 : 0);

// Minimal example: use StarDS from JavaScript via the WebAssembly embind module.
//
// Run:  node StarDS/tests/wasm/example.mjs   (after run_embind_example.sh builds it)
//
// The module is built as a factory (MODULARIZE=1, EXPORT_ES6=1). Because opens/
// reads perform network fetches that block via ASYNCIFY, the embind methods that
// touch the network return Promises — so we await them.
import createStardsModule from '../../../build-embind/stards.mjs';

const URL =
  'https://asc-isisdata.s3.us-west-2.amazonaws.com/cnf_test_data/largenet.stards';

const Module = await createStardsModule();

// Open the dataset (ranged GETs over fetch()).
const ds = await new Module.Dataset(URL);

// keys()/shape() return real JS Arrays (from val::array()), so use length/index.
const keys = ds.keys();
console.log(`opened OK — ${keys.length} array keys`);

// Print the first few keys with their dtype + shape.
for (let i = 0; i < Math.min(5, keys.length); i++) {
  const k = keys[i];
  const dims = ds.shape(k);
  console.log(`  ${k}  (${ds.dtype(k)})  shape=[${dims.join(',')}]`);
}

// Read one array back as a JS typed array.
const key = keys[1];
const arr = await ds.get(key);
console.log(`\nread "${key}": ${arr.constructor.name}(${arr.length}), head =`,
            Array.from(arr.slice(0, 6)));

console.log(`network requests: ${ds.networkRequests()}`);

// embind objects are manually managed — free them.
ds.delete();

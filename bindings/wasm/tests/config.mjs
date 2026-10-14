// StarConfig / create() test for the StarDS WASM module: create a dataset with an
// explicit compression codec + block size, write data, reopen, and confirm the
// data round-trips and (for the block-shuffle codec) stays sliceable.
//
// Build first, then run from the repo root:
//   cmake --build build --target wasm_tests
//   node build/tests/config.mjs
//
// Runs entirely in MEMFS — no network, nothing left on disk.
import { loadStarDS } from '../stards.mjs';

const { Module, create, Dataset } = await loadStarDS();
let fails = 0;
const ok = (label, cond, extra = '') => {
  if (!cond) fails++;
  console.log(`${cond ? 'OK ' : 'XX '} ${label}${extra ? '  ' + extra : ''}`);
};
const j = (x) => JSON.stringify(x, (k, v) => (typeof v === 'bigint' ? v + 'n' : v));

// Compression enum exposes only codecs this build can run (NONE + GZIP* on WASM).
const names = Object.keys(Module.Compression);
ok('Compression has NONE + GZIP variants', ['NONE', 'GZIP', 'GZIP_SHUFFLE', 'GZIP_SHUFFLE_BLOCK'].every((n) => names.includes(n)), `-> [${names.join(',')}]`);
ok('LZ4/ZSTD not exposed on WASM', !names.some((n) => n.includes('LZ4') || n === 'ZSTD'));

const data = Float32Array.from({ length: 4096 }, (_, i) => Math.sin(i / 10));

async function roundtrip(codecName, sliceable) {
  const cfg = new Module.StarConfig();
  cfg.compression = Module.Compression[codecName];
  cfg.blockSize = 64 * 1024;
  const w = await create(`${codecName}.stards`, cfg);
  cfg.delete();
  w.put('sig', data, [data.length], 'float32');
  w.flush();
  w.close();
  w.delete();

  const r = await new Dataset(`${codecName}.stards`, 'r');
  ok(`${codecName}: data round-trips`, j([...r.get('sig')]) === j([...data]));
  ok(`${codecName}: isSliceable == ${sliceable}`, r.isSliceable('sig') === sliceable);
  if (sliceable) {
    ok(`${codecName}: getSlice window`, j([...r.getSlice('sig', 10, 5)]) === j([...data.slice(10, 15)]));
  }
  r.delete();
}

await roundtrip('NONE', true);
await roundtrip('GZIP_SHUFFLE_BLOCK', true); // block variant stays sliceable
await roundtrip('GZIP_SHUFFLE', false); // legacy whole-array: not sliceable

// You can't smuggle in an unsupported codec via a raw ordinal: embind maps a
// number that isn't a registered enum member to the zero value (NONE), so setting
// the LZ4 ordinal degrades to NONE rather than producing an unreadable file.
{
  const cfg = new Module.StarConfig();
  cfg.compression = Module.Compression.GZIP;
  cfg.compression = 3; // LZ4 ordinal — not a registered member; falls back to NONE
  ok('bogus codec falls back to NONE', cfg.compression === Module.Compression.NONE);
  cfg.delete();
}

console.log(fails ? `\n${fails} FAILURE(S)` : '\nALL PASS');
process.exit(fails ? 1 : 0);

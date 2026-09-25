// Run every *.mjs test in this directory (except this runner) and report a tally.
// Each test is a standalone script that exits non-zero on failure, so we just spawn
// them and collect exit codes.
//
//   node build/tests/run_all.mjs
//
// Requires the module to be built first:
//   cmake --build build --target wasm_tests
import { readdirSync } from 'node:fs';
import { spawnSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';
import { dirname, join, basename } from 'node:path';

const here = dirname(fileURLToPath(import.meta.url));
const self = basename(fileURLToPath(import.meta.url));
const files = readdirSync(here)
  .filter((f) => f.endsWith('.mjs') && f !== self)
  .sort();

const failed = [];
for (const f of files) {
  console.log(`\n=== ${f} ===`);
  // Same node binary; inherit stdio so each test's OK/XX lines stream through.
  const r = spawnSync(process.execPath, [join(here, f)], { stdio: 'inherit' });
  if (r.status !== 0) failed.push(f);
}

console.log(`\n${'='.repeat(48)}`);
if (failed.length) {
  console.log(`${failed.length}/${files.length} test file(s) FAILED: ${failed.join(', ')}`);
  process.exit(1);
}
console.log(`All ${files.length} test files passed.`);

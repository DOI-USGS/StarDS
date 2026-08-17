// Smoke-test the homepage hero's DATA LAYER (hero.js's makeNetApi) against the
// real remote .stards net, under node — no browser, no WebGL, no rendering.
//
// What it covers: that StarDS's WASM build in docs/assets/wasm/ still satisfies
// what the renderer asks of it — the polyline overview reads and dequantizes to
// plausible on-ellipsoid vertices, a windowed point read returns interleaved XYZ
// on the same body, and the two agree line-by-line. That is exactly the contract
// that breaks silently in a browser (the hero just shows its placeholder globe).
//
// NETWORK TEST: it streams byte ranges from a public S3 bucket, so it needs
// outbound HTTPS. Run it after rebuilding the WASM module:
//
//   node docs-site/tests/hero_net_smoke.mjs
import { readFile } from "node:fs/promises";
import { fileURLToPath } from "node:url";

const HERO = new URL("../docs/javascripts/hero.js", import.meta.url);
const WASM_DIR = new URL("../docs/assets/wasm/", import.meta.url);
const NET =
  "/vsicurl/https://asc-isisdata.s3.us-west-2.amazonaws.com/cnf_test_data/largenet_lines.stards";

// Mars, from the net's header (the fallbacks hero.js uses agree with it).
const RADIUS_A = 3396190, RADIUS_C = 3376200;

let failures = 0;
function check(name, ok, detail = "") {
  console.log(`${ok ? "ok  " : "FAIL"}  ${name}${detail ? "  — " + detail : ""}`);
  if (!ok) failures++;
}

// hero.js's module body kicks its own loads off on import: Three.js from a CDN
// (rejection is caught there) and the WASM module through fetch(). Under node the
// module URL is a file:// URL, so serve those from disk; `caches` is absent, which
// hero.js already treats as "no cache".
const realFetch = globalThis.fetch;
globalThis.fetch = async (url, opts) => {
  const s = String(url && url.url ? url.url : url);
  if (s.startsWith("file://")) {
    const body = await readFile(fileURLToPath(s));
    return new Response(body, { headers: { "content-type": "application/wasm" } });
  }
  return realFetch(url, opts);
};
// Enough DOM for hero.js's bootstrap to find no hero and stay inert.
globalThis.document = { readyState: "loading", addEventListener() {}, querySelector: () => null };

const { makeNetApi } = await import(HERO.href);
const { default: createStardsModule } = await import(new URL("stards.mjs", WASM_DIR).href);

const api = makeNetApi(await createStardsModule({ print: () => {}, printErr: () => {} }));

// --- Polyline overview -------------------------------------------------------
const lh = await api.openLines(NET);
const L = await api.linesCount(lh);
check("lines layer present", L > 0, `${L} lines`);

const lines = await api.linesReadAll(lh);
await api.closeLines(lh);

check("voff is the CSR of the vertex buffer", lines.voff.length === L + 1 &&
  lines.voff[L] * 3 === lines.positions.length, `${lines.voff[L]} vertices`);
check("point windows sized per line", lines.rangeStart.length === L && lines.rangeCount.length === L);

// Every dequantized vertex must sit on the ellipsoid (polar <= |p| <= equatorial).
let minR = Infinity, maxR = 0;
for (let i = 0; i < lines.voff[L]; i++) {
  const x = lines.positions[i * 3], y = lines.positions[i * 3 + 1], z = lines.positions[i * 3 + 2];
  const r = Math.hypot(x, y, z);
  if (r < minR) minR = r;
  if (r > maxR) maxR = r;
}
check("overview vertices lie on the body", minR > RADIUS_C - 1 && maxR < RADIUS_A + 1,
  `radii ${minR.toFixed(0)}..${maxR.toFixed(0)} m`);

// --- Windowed point reads ----------------------------------------------------
const ph = await api.openPointsXYZ(NET);
const total = await api.pointsCount(ph);
check("point count from the index", total > 0, `${total.toLocaleString()} points`);

const batch = await api.pointsReadXYZ(ph, 0, 4096);
check("batch is interleaved XYZ", batch.count === 4096 && batch.positions.length === batch.count * 3);
check("batch radius is the body radius", Math.abs(batch.radius - RADIUS_A) < 50000,
  `${batch.radius.toFixed(0)} m`);

// A short read past the end must clamp, not throw.
const tail = await api.pointsReadXYZ(ph, total - 10, 4096);
check("read past the end clamps", tail.count === 10);

// The overview and the real points must describe the same geometry: line i's first
// vertex should be near the first point of its [rangeStart,rangeCount) window.
// Tolerance is generous — the overview is quantized (~325 m/code) and flattened
// onto the ellipsoid (heights dropped).
let worst = 0;
for (const i of [0, 1, 2, (L / 2) | 0, L - 1]) {
  const n = lines.rangeCount[i];
  if (!n) continue;
  const v = lines.voff[i];
  const p = await api.pointsReadXYZ(ph, lines.rangeStart[i], 1);
  if (!p.count) continue;
  const d = Math.hypot(
    lines.positions[v * 3] - p.positions[0],
    lines.positions[v * 3 + 1] - p.positions[1],
    lines.positions[v * 3 + 2] - p.positions[2]
  );
  if (d > worst) worst = d;
}
check("overview vertices track their real points", worst < 20000, `worst ${worst.toFixed(0)} m`);

api.closePointsXYZ(ph);
console.log(failures ? `\n${failures} check(s) failed` : "\nall checks passed");
process.exit(failures ? 1 : 0);

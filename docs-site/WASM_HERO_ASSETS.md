# StarDS WASM module for the homepage hero

`docs/assets/wasm/stards.mjs` + `stards.wasm` are a build of
`StarDS/tests/wasm/stards_embind.cpp` — StarDS's own embind wrapper — checked in
so the homepage hero (`docs/javascripts/hero.js`) can stream and render a real
control network on the landing page with no build step at docs-deploy time.

The module exposes StarDS, not a hero API: a `Dataset` class with
`keys` / `dtype` / `shape` / `metaString` / `get` / `getSlice` / `getSliceXYZ`.
Everything control-net-specific — which keys hold the adjusted points, how the
`lines` layer's quantized (lon,lat) pairs become XYZ vertices — lives in
`makeNetApi()` in `hero.js`. `getSliceXYZ` is the streaming primitive: it reads the
same window from three columns and interleaves it into one `Float32Array`, so a
batch costs only the compressed blocks that cover that window.

The data itself is *not* vendored: the hero streams a ~2 GB `.stards` net over
`/vsicurl/` from a public S3 bucket, reading only the byte ranges it needs (the
header/index on open, then the covering compressed blocks per batch).

## Rebuilding

Needs emscripten (`em++`) with a matching binaryen — `conda create -n stards-wasm
-c conda-forge emscripten=3.1.58` works; the 4.0.9 conda package ships a binaryen
too old for its own linker flags.

```sh
StarDS/tests/wasm/run_embind_example.sh          # -> build-embind/stards.mjs + .wasm
cp build-embind/stards.{mjs,wasm} docs-site/docs/assets/wasm/
node docs-site/tests/hero_net_smoke.mjs          # data-layer check against the real net
```

The script also runs `StarDS/tests/wasm/example.mjs`, which exercises the generic
`Dataset` surface. `hero_net_smoke.mjs` covers what the hero depends on: the
overview layer dequantizes to on-ellipsoid vertices, windowed point reads come back
as interleaved XYZ on the same body, short reads at the end clamp, and each line's
first vertex tracks the first real point of its window. Both need outbound HTTPS.

Bump `WASM_CACHE_NAME` in `hero.js` if a rebuild has to reach clients that may hold
a cached copy under an unchanged ETag.

## Load-time behaviour

`stards.wasm` is ~720 KB (~215 KB gzipped on the wire; the JS loader adds ~120 KB).
Three things in the code exist to keep it off the critical path:

- **The hero never waits on it.** Only Three.js gates the first frame; the
  placeholder globe renders while the module is still in flight, and the
  WASM-backed layers attach when it lands.
- **Cache Storage + ETag revalidation** (`fetchWasmBinary` in `hero.js`, wired in
  through Emscripten's `instantiateWasm` hook). Pages serves the binary with
  `cache-control: max-age=600`, so ten minutes after a visit the HTTP cache treats
  it as stale; instead a returning visitor spends one conditional GET that comes
  back `304`.
- **`priority: "low"` on that fetch**, plus `preconnect` / `modulepreload` hints in
  `overrides/home.html`. The two loads run in parallel, but Three.js — the one the
  first frame needs — wins the bandwidth.

For scale: the hero previously ran on a vendored Miniset build, 36 MB (~9.6 MB
gzipped), because that module carries the whole GDAL / PROJ / SQLite / GeoTIFF
stack (24 MB code + 12 MB embedded `proj.db` and GDAL data) that none of this
needs.

## Notes

- The hero module is loaded on the homepage only (from `overrides/home.html`,
  not `extra_javascript`), so no other page of the site pays for any of this.
- `hero.js` resolves both files relative to its own `import.meta.url`, so a
  subdirectory deploy (e.g. GitHub Pages under `/StarDS/`) works unchanged. Set
  `window.STARDS_HERO_WASM_BASE` to serve them from elsewhere.
- If the module fails to load, the hero still renders its gradient, text and
  placeholder globe — just without the point cloud.

/**
 * StarDS docs — Three.js hero banner (streaming point-cloud globe).
 *
 * The renderer began as a port of the Miniset docs hero (docs/javascripts/hero.js
 * there); the data layer is now StarDS's own WASM build (see WASM_JS_URL and
 * makeNetApi below), so the whole page runs on this repo's code.
 *
 * Renders real control-network points (POINTS, not measures) from a normalized
 * StarDS net as a rotating globe of 3D circles behind the hero text. The points
 * are PARSED BY StarDS ITSELF, streamed straight from the REMOTE net over
 * /vsicurl/: the page loads StarDS's WASM module and opens a dataset on the S3
 * URL, which reads only the byte ranges it needs — the header/index on open, then
 * the covering compressed blocks per windowed read — so the ~2 GB file is never
 * downloaded whole. Points stream in over time (batch by batch) as the globe
 * turns; each batch fades in (rather than blinking into place) via a per-point
 * "birth time" the shader ramps to full opacity.
 *
 * The WASM module is built with -sASYNCIFY, so the StarDS reads that fetch
 * (Dataset.get / getSlice / getSliceXYZ) SUSPEND and return Promises — they
 * are awaited here. The read loop keeps one batch in flight at a time and applies
 * it when it resolves, so the render/rotation never blocks on the network.
 *
 * Requires the S3 bucket to serve CORS (Access-Control-Allow-Origin + expose
 * Range/Content-Range) so the browser fetch() succeeds from the docs origin.
 *
 * Three.js is pulled from a CDN as an ES module so the static docs need no build
 * step (same no-bundler approach as the WASM playground).
 *
 * STARTUP ORDER (why nothing here is awaited up front): the render must appear
 * IMMEDIATELY, so boot() waits only on Three.js — never on the WASM module. The
 * placeholder globe is pure geometry (no network), so it draws on the first frame,
 * and the WASM-backed layers (overview, then the real streamed points) attach
 * themselves later through the promises in `src`. See openStream() and boot().
 */

// Three.js, loaded LAZILY via dynamic import rather than the top-level static
// import this file used to carry (`import * as THREE from <cdn>`). Two reasons:
//
//   1. boot() can then await Three.js EXPLICITLY and nothing else, which is what
//      makes "render immediately, attach the WASM layers later" a visible contract
//      instead of an accident of import order. With a static import, this module's
//      body could not run at all until the ~600 KB CDN build had arrived, so the
//      load ordering below wasn't ours to choose.
//   2. A CDN failure becomes catchable. A failed static import aborts the whole
//      module — no code of ours ever runs and the hero silently stays blank;
//      here the rejection surfaces in boot()'s catch as a console warning.
//
// THREE is referenced only from inside functions, every one of which runs after
// loadThree() has resolved, so a module-scope binding filled in at that point is
// equivalent to the namespace import. Memoized: one fetch per page.
const THREE_URL = "https://unpkg.com/three@0.160.0/build/three.module.js";
let THREE = null;
let threePromise = null;
function loadThree() {
  if (threePromise) return threePromise;
  threePromise = import(/* webpackIgnore: true */ THREE_URL)
    .then((mod) => { THREE = mod; return mod; })
    .catch((err) => { threePromise = null; throw err; });
  return threePromise;
}

/* ==========================================================================
   TUNING LEVERS — tweak these live; everything visual keys off this object.
   After changing one from the console, call window.msHero.apply().
   ========================================================================== */
const CONFIG = {
  // Camera / framing.
  zoom: 1.5,          // >1 zooms IN (points get bigger + denser on screen)
  offsetX: 0.1,       // fraction of viewport to shove the globe RIGHT
                       //   (0 = centered, 0.4–0.5 packs it into the right 2/3)
  offsetY: 0.0,        // vertical nudge (fraction of viewport; + is up)
  autoRotate: true,    // slow spin so the globe reads as 3D
  rotateSpeed: 0.08,  // radians / second — also sets the load rate (see below)

  // Virtual trackpad (bottom-right) — a mini touchpad you drag on to orbit the
  // globe by hand. Dragging maps the pointer's MOVEMENT directly to rotation
  // (Δx → yaw, Δy → pitch), so it feels like spinning the sphere with a finger;
  // it only rotates while the pointer is held down INSIDE the pad. This orbits a
  // group ABOVE the auto-spin, so it does NOT disturb the spin that paces
  // streaming, nor the camera framing (setViewOffset right-shift is intact).
  trackpad: true,          // show the orbit trackpad when supported (pointer input)
  trackpadSensitivity: 0.9,// radians of orbit per 1 pad-width of pointer travel
                           // (orbit is a gimbal-lock-free quaternion trackball, so
                           //  there's no pitch limit — the globe can tumble fully)

  // Orientation. The body's poles are on its Z axis, so we stand the globe up
  // (pole -> screen up) and spin AROUND the pole, then lean it by `axisTilt` so
  // it rotates on a slightly tilted axis like Mars (obliquity ≈ 25°). The tilt
  // leans toward/away from the camera about the screen X axis; `axisTiltRoll`
  // spins that lean around so the pole tips left/right instead of front/back.
  axisTilt: 45,        // degrees the spin axis leans from straight-up (Mars ≈ 25°)
  axisTiltRoll: 0,     // degrees to roll the lean around vertical (0 = tips back)

  // Points.
  dotSize: 1.3,        // on-screen diameter of each circle, in device px
  dotColor: 0xe7faf3,  // circle colour
  dotOpacity: 1.0,    // opacity of a point once faded in (uniform front-to-back)

  // Depth cue — a light DISTANCE FOG (not per-point opacity). Far points blend
  // toward the fog colour so you can still faintly tell they're on the back of
  // the shell, but they stay understated and don't wash out the front.
  fogColor: 0x162b67,  // colour distant points recede toward (≈ the backdrop)
  fogStrength: 1.0,    // 0 = no fog, 1 = back points fully become fogColor.
                       //   ~0.9 keeps the back very faint but still perceptible.
  fogFalloff: 1.0,     // how quickly the fog kicks in with depth (exponent).
                       //   1 = linear; >1 keeps the front clear and only fogs
                       //   the deep back (fog "kicks in" later); <1 fogs sooner.

  // Render edge — the vertical LINE (from the left) where the cloud starts to
  // appear. The canvas is masked to transparent left of `renderEdge`, ramping to
  // fully visible over `renderEdgeSoftness`, keeping the headline area clear.
  renderEdge: 0.01,        // fraction of hero width where points begin to render
  renderEdgeSoftness: 0.01, // fraction of width the fade-in spans past the edge

  // Streaming — the cloud fills as the globe turns.
  //
  // Points are stored sorted by azimuth (a vertical wedge per file window), so we
  // tie the load cursor to rotation: whatever wedge has rotated to the FRONT is
  // loaded, so front-facing points arrive first and one full turn loads the file.
  // Slowing rotateSpeed therefore also slows the load rate — they're the same knob.
  maxPoints: 3000000,      // hard cap on REAL points streamed to the GPU: the
                           //   stream STOPS once this many have loaded (source
                           //   holds ~9.4M). Bounds memory + fetching; regions past
                           //   the cap keep the line overview instead of real points.
                           //   The "Load all points" control lifts this on request
                           //   (msHero.loadAllPoints()), regrowing the buffers.
  // ADAPTIVE batch size: every read costs 3 fixed HTTP round-trips (one per X/Y/Z
  // array) regardless of batch size, so per-point cost falls sharply as the batch
  // grows (~134µs/pt at 30k → ~34µs/pt at 240k). But a huge first read stalls the
  // first paint. So start small (streamBatchMin) for an instant seed, then ramp
  // ×streamBatchGrow each read up to streamBatchMax — fast first frame, cheap
  // steady state. (streamBatchSize kept as the starting/back-compat value.)
  streamBatchSize: 30000,  // starting batch size (first reads, quick first paint)
  streamBatchMin: 30000,   // floor
  streamBatchMax: 240000,  // ceiling (bigger = cheaper/pt but longer per read)
  streamBatchGrow: 1.6,    // multiply the batch each read until it hits the max
  fadeDurationMs: 1600,    // how long each point takes to ramp to full opacity
  seedPoints: 50000,       // load this many immediately at startup so the front
                           //   is populated at once (no waiting for the first read)
  loadLeadTurns: 0.0,      // load this fraction of a turn AHEAD of the front.
                           //   0 = new points appear dead-front (on camera); a
                           //   positive lead spawns them off to the side, where
                           //   the right-shoved globe can clip them off-screen.
  // Fetch budget: stop streaming real points after `loadTurns` full rotations
  // (in addition to the maxPoints cap), so a user who never zooms doesn't pay to
  // fetch the whole ~9.4M cloud. 0 = unlimited (fill until maxPoints).
  loadTurns: 3.0,          // stop the point stream after this many globe turns
                           //   (also lifted by "Load all points")

  // Polyline LOD (cnet/3 "lines" layer) — the preferred track overview. The net's
  // geometric filaments are drawn as ribbons IMMEDIATELY (from a compact
  // on-ellipsoid model, no real points sent yet), then each line's real points
  // stream in and the line crossfades out as its points fade in. Falls back to
  // the GMM splats, then to the plain stream, if no lines layer exists.
  showLines: true,         // draw the polyline overview when a lines layer exists
  lineColor: 0x8fb7ff,     // sampled-point tint (match dotColor for a seamless look)
  lineOpacity: 0.55,       // sampled-point brightness (additive)
  lineFadeOutMs: 900,      // how long a line fades out once its real points arrive
  // The polylines are rendered as POINTS sampled along each line (disc sprites,
  // like the real cloud) rather than as lines, so the overview reads as points.
  lineSampleSpacing: 1500, // metres between sampled points along a line
  lineMaxSamples: 100000,  // cap on total sampled points (memory/perf guard)
  lineDotSize: 1.3,        // on-screen diameter of a sampled point (device px)

  // Gaussian-splat LOD (cnet/3 "summary" layer) — fallback overview when there's
  // no "lines" layer. Reconstructs the mixture as a synthetic point cloud.
  showSplats: true,        // reconstruct + render the GMM when a summary exists
  gmmPoints: 250000,       // total synthetic points sampled from the mixture
  gmmColor: 0x8fb7ff,      // synthetic-point tint (distinct from real dotColor)
  gmmOpacity: 0.5,         // synthetic-point brightness (additive)
  gmmFadeOutMs: 900,       // how long a splat's synthetic points take to fade out
                           //   once its real points arrive

  // Placeholder globe — a low-density REGULAR point grid on the Mars ellipsoid,
  // shown from the first frame (no network) so the viewport is never empty while
  // the overview layer is being opened/streamed. It fades out the moment the
  // overview (lines/GMM) is ready and rendered. Styled to match the overview
  // (same colour/opacity/fog), just sparser, so the handoff is seamless.
  showPlaceholder: true,     // draw the regular ellipsoid grid until the overview lands
  placeholderRings: 256,      // number of latitude rings pole-to-pole (grid density)
  placeholderColor: 0x8fb7ff,// match lineColor/gmmColor for a seamless handoff
  placeholderOpacity: 0.9,   // brightness (additive), a touch under the overview
  placeholderDotSize: 1.5,   // on-screen diameter of a grid point (device px)
  placeholderFadeOutMs: 700, // fade-out once the overview is ready and displayed
  // Mars IAU biaxial ellipsoid radii (metres) the grid is generated on; matches
  // the net's on-ellipsoid line model (a = equatorial, c = polar).
  placeholderRadiusA: 3396190,
  placeholderRadiusC: 3376200,

  // Backdrop.
  clearAlpha: 0.0,     // 0 = transparent (the CSS gradient shows through)
};

// StarDS's own WASM build and the .stards net.
//
// The module is StarDS/tests/wasm/stards_embind.cpp compiled with -sASYNCIFY (so
// the ranged /vsicurl/ reads can suspend and hand JS a Promise) and checked in
// under docs/assets/wasm/ — see docs-site/WASM_HERO_ASSETS.md for the build
// command and how to refresh it. It exposes StarDS itself, not a hero API: a
// `Dataset` class with keys / dtype / shape / metaString / get / getSlice /
// getSliceXYZ. Everything cnet-shaped — which keys hold the points, how the
// polyline overview's quantized (lon,lat) become XYZ — is assembled here in JS by
// makeNetApi(), so the C++ binding stays a general StarDS surface.
//
// `window.STARDS_HERO_WASM_BASE` overrides the location (e.g. to serve the
// module from a CDN instead of the docs origin). If the module fails to load,
// loadStards() rejects; the render is never gated on it, so the hero keeps
// spinning its placeholder globe over the gradient + text — just without the
// streamed points — and boot() retries on the next navigation.
const WASM_BASE =
  (typeof window !== "undefined" && window.STARDS_HERO_WASM_BASE) ||
  new URL("../assets/wasm/", import.meta.url).href;
const WASM_JS_URL = new URL("stards.mjs", WASM_BASE).href;
const WASM_BINARY_URL = new URL("stards.wasm", WASM_BASE).href;
// Remote control net, read over /vsicurl/ — the module fetches only the byte
// ranges it needs (header + the covering blocks per batch), never the whole
// ~2 GB file. Requires the bucket to serve CORS (Allow-Origin + Range /
// Content-Range) so the browser fetch() succeeds from the docs origin.
const STARDS_URL =
  "/vsicurl/https://asc-isisdata.s3.us-west-2.amazonaws.com/cnf_test_data/largenet_lines.stards";

/* --------------------------------------------------------------------------
   WASM binary caching.

   The binary is ~720 KB (~215 KB gzipped on the wire). GitHub Pages serves it
   with `cache-control: max-age=600`, so ten minutes after a visit the browser's
   HTTP cache treats it as stale and a returning visitor pays for the whole
   transfer again — and that short TTL also keeps V8's own compiled-code cache
   from surviving. We can't set headers on Pages, so we keep our own copy in
   Cache Storage and REVALIDATE it with an ETag instead:

     * first visit          — plain fetch, streamed straight into the compiler,
                              a clone stored in Cache Storage. Same cost as before.
     * later visits         — conditional GET with `If-None-Match`. GitHub Pages
                              answers 304 (empty body, one round trip) and we
                              instantiate from the cached bytes.
     * changed deploy       — the ETag differs, so the 200 response streams in
                              and replaces the cached entry.
     * offline / 5xx        — fall back to the cached copy if we have one.

   Compilation itself is NOT cached: it measures a few ms on a module this size
   (V8 tiers it lazily), so an IndexedDB compiled-module cache would add real
   complexity for no measurable gain.

   This mattered far more when the hero ran on miniset's 36 MB module; at ~215 KB
   the saving is one small transfer per visit. It is kept because it costs one
   conditional GET and keeps the first frame's bandwidth for Three.js.

   Note for a cross-origin `window.STARDS_HERO_WASM_BASE`: `If-None-Match` is not
   a CORS-safelisted header, so the revalidation would need a preflight (and the
   server would have to allow it). Everything degrades to the plain refetch path
   below if that fails, but a CDN base is better off served with a long/immutable
   `cache-control`, which makes this cache redundant anyway.
   -------------------------------------------------------------------------- */

// Bump the suffix to force every client to re-download (e.g. if a build is
// published under an unchanged ETag).
const WASM_CACHE_NAME = "stards-hero-wasm-v2";

// Cache Storage is absent in insecure contexts and in Safari private browsing;
// every caller treats null as "no cache" and just goes to the network.
async function openWasmCache() {
  if (typeof caches === "undefined") return null;
  try {
    return await caches.open(WASM_CACHE_NAME);
  } catch (_) {
    return null;
  }
}

/**
 * Resolve the WASM binary to a Response whose body can be handed to
 * instantiateStreaming — from Cache Storage when our copy is still current.
 * @returns {Promise<Response>}
 */
async function fetchWasmBinary() {
  const cache = await openWasmCache();
  const cached = cache ? await cache.match(WASM_BINARY_URL) : null;
  const etag = cached && cached.headers.get("ETag");

  // `priority: "low"` (Fetch Priority; ignored where unsupported) keeps this
  // multi-MB transfer from competing with the Three.js build that the first
  // frame actually needs — see the load-order note below.
  const res = await fetch(WASM_BINARY_URL, {
    credentials: "same-origin",
    priority: "low",
    headers: etag ? { "If-None-Match": etag } : undefined,
  });

  // Not modified: our cached bytes are current.
  if (res.status === 304 && cached) return cached;
  // Server trouble (or offline): a stale-but-working copy beats no hero points.
  if (!res.ok) {
    if (cached) return cached;
    throw new Error(`wasm fetch failed: ${res.status} ${res.statusText}`);
  }

  // Store a clone for next time. NOT awaited: cache.put() resolves only once it
  // has read the whole body, and awaiting it would serialize the download ahead
  // of compilation — exactly the streaming overlap we want to keep. The clone
  // and the returned response drain the same stream concurrently.
  if (cache) {
    cache.put(WASM_BINARY_URL, res.clone()).catch(() => {});
  }
  return res;
}

/**
 * Emscripten's `instantiateWasm` hook: takes over locating/compiling the binary
 * so the cache path above is used instead of the loader's own unconditional
 * fetch. Returning `{}` tells the loader the work is asynchronous and that the
 * instance will arrive via `receiveInstance`.
 *
 * `onFatal` is the escape hatch for a total failure. The loader only rejects its
 * own promise if this hook throws SYNCHRONOUSLY, so an async failure would
 * otherwise leave the factory promise pending forever — and the hero would never
 * learn the module was gone (no `src.failed`, no retry on the next navigation).
 */
function makeInstantiateWasm(onFatal) {
  return (imports, receiveInstance) => {
    (async () => {
      try {
        const res = await fetchWasmBinary();
        const { instance, module } = await WebAssembly.instantiateStreaming(res, imports);
        receiveInstance(instance, module);
        return;
      } catch (err) {
        // Streaming compilation also lands here if a custom WASM_BASE serves the
        // binary without `content-type: application/wasm`.
        console.warn("[StarDS hero] cached WASM load failed, refetching:", err);
      }
      const res = await fetch(WASM_BINARY_URL, { credentials: "same-origin" });
      if (!res.ok) throw new Error(`wasm refetch failed: ${res.status}`);
      const { instance, module } = await WebAssembly.instantiate(await res.arrayBuffer(), imports);
      receiveInstance(instance, module);
    })().catch(onFatal);
    return {};
  };
}

let stardsPromise = null;
function loadStards() {
  if (stardsPromise) return stardsPromise;
  // `fatal` is how an async instantiate failure reaches this promise (see
  // makeInstantiateWasm): racing it against the factory turns a would-be
  // forever-pending module into a rejection the hero can recover from.
  let onFatal;
  const fatal = new Promise((_, reject) => { onFatal = reject; });
  stardsPromise = import(/* webpackIgnore: true */ WASM_JS_URL)
    .then(({ default: StardsFactory }) =>
      Promise.race([
        StardsFactory({
          locateFile: (p) => (p.endsWith(".wasm") ? WASM_BINARY_URL : p),
          instantiateWasm: makeInstantiateWasm(onFatal),
          print: () => {},
          printErr: () => {},
        }),
        fatal,
      ])
    )
    .then(makeNetApi)
    .catch((err) => { stardsPromise = null; throw err; });
  return stardsPromise;
}

// Kick BOTH loads off here, at module-evaluation time (rather than waiting for
// boot()), and IN PARALLEL.
//
// Three.js is all the placeholder globe needs, and the placeholder is what must
// appear immediately — so it must not have to share the link with a ~36 MB
// download. This used to be enforced by CHAINING the WASM load behind Three.js,
// which also put the whole Three.js round trip (cross-origin CDN: DNS + TLS +
// ~600 KB) on the WASM module's critical path. Priority does that job better:
// the binary is fetched with `priority: "low"` (see fetchWasmBinary), so the
// browser lets Three.js win the bandwidth while the big transfer still gets to
// start now rather than one round trip later. home.html's `preconnect` /
// `modulepreload` hints shorten the other end of the same path.
//
// Both loaders are memoized, so boot()/openStream() reuse these exact promises.
// The .catch()es merely mark rejections as observed (no unhandled-rejection noise);
// each failure is reported and handled at its real use site.
loadThree().catch(() => {});
loadStards().catch(() => {});

/* --------------------------------------------------------------------------
   The cnet layer: StarDS keys -> what the renderer wants.

   The WASM module hands us plain StarDS (`Dataset`), so this is where the
   control-net conventions live:

     * POINTS — body-centred body-fixed metres in three float64 columns
       (`p.adjustedX/Y/Z` in a normalized net, `adjX/adjY/adjZ` in a compact
       XYZ-only one). A batch is one getSliceXYZ call: three windowed reads,
       interleaved into Float32 XYZ inside WASM. Only the compressed blocks that
       cover the window are fetched, which is what makes streaming a ~2 GB file
       from a static bucket possible at all.

     * OVERVIEW — the optional "lines" layer, a coarse polyline sketch of the
       cloud written as quantized on-ellipsoid (lon,lat) int16 pairs plus a CSR
       vertex offset array, and per-line [rangeStart,rangeCount) windows into the
       point arrays for the drill-down. It is small enough to read whole, and it
       is what the hero shows while the real points stream in behind it. The
       quantization is a fixed int16 code over the full angular range; heights are
       dropped (the sketch sits on the ellipsoid), whose radii come from the net's
       own header when it carries them.

   Both are handed back through the same three-call handle shape the renderer
   uses — open / count / read — so the pumps stay unaware of any of this.
   -------------------------------------------------------------------------- */

// Layer-qualified key prefix used by StarDS for a named layer's arrays.
const LINES_LAYER = "__layer_lines__:";
// int16 code <-> radians, matching the writer: the full lon range spans the whole
// 65535-code space, latitude likewise over its (half-as-wide) range.
const LON_SCALE = 65535 / (2 * Math.PI);
const LAT_SCALE = 65535 / Math.PI;

/**
 * Wrap the raw StarDS module in the point/overview API the renderer consumes.
 *
 * Every method that touches the file is async: under -sASYNCIFY a StarDS read
 * suspends into JS's fetch() and returns a Promise. ASYNCIFY permits only ONE
 * suspension in flight, so callers must not overlap these — openStream() and the
 * pumps already serialize them.
 *
 * @param {any} Module the instantiated Emscripten module (embind `Dataset`)
 */
function makeNetApi(Module) {
  // One open Dataset per URL: opening costs a ranged GET for the header/index, and
  // the overview read and the point stream both want the same file. Handles below
  // are plain objects referring back to this entry.
  const datasets = new Map();

  async function dataset(url) {
    let entry = datasets.get(url);
    if (!entry) {
      const ds = await new Module.Dataset(url);
      entry = { ds, keys: new Set(ds.keys()) };
      datasets.set(url, entry);
    }
    return entry;
  }

  function dispose(url) {
    const entry = datasets.get(url);
    if (!entry) return;
    datasets.delete(url);
    try { entry.ds.delete(); } catch (_) {}
  }

  // The two column-naming conventions for adjusted body-fixed positions.
  const POINT_KEYS = [
    ["p.adjustedX", "p.adjustedY", "p.adjustedZ"],
    ["adjX", "adjY", "adjZ"],
  ];

  return {
    // --- Points --------------------------------------------------------------
    async openPointsXYZ(url) {
      const { ds, keys } = await dataset(url);
      const xyz = POINT_KEYS.find((k) => k.every((key) => keys.has(key)));
      if (!xyz) throw new Error("no adjusted XYZ columns in this net");
      const total = (ds.shape(xyz[0])[0] | 0) || 0;
      return { url, ds, xyz, total };
    },

    // Synchronous (the count came from the index at open time), but every caller
    // awaits it — the renderer treats the whole API as async.
    pointsCount(h) {
      return h.total;
    },

    // Window [start, start+count) as { positions: Float32Array(n*3), count, radius }.
    // `radius` is the largest |p| in the batch: the renderer frames the camera off
    // the first batch it receives, and on a whole-globe cloud that is already the
    // body radius.
    async pointsReadXYZ(h, start, count) {
      const positions = await h.ds.getSliceXYZ(h.xyz[0], h.xyz[1], h.xyz[2], start, count);
      const n = (positions.length / 3) | 0;
      let r2 = 0;
      for (let i = 0; i < n; i++) {
        const x = positions[i * 3], y = positions[i * 3 + 1], z = positions[i * 3 + 2];
        const d = x * x + y * y + z * z;
        if (d > r2) r2 = d;
      }
      return { positions, count: n, radius: Math.sqrt(r2) };
    },

    // The renderer closes the point stream when it tears the scene down, and it is
    // the last reader — so this is where the shared Dataset is actually released.
    closePointsXYZ(h) {
      if (h && h.url) dispose(h.url);
    },

    // --- Polyline overview ---------------------------------------------------
    async openLines(url) {
      const { ds, keys } = await dataset(url);
      const k = {
        qlon: LINES_LAYER + "lines.qlon",
        qlat: LINES_LAYER + "lines.qlat",
        voff: LINES_LAYER + "lines.voff",
        rangeStart: LINES_LAYER + "lines.rangeStart",
        rangeCount: LINES_LAYER + "lines.rangeCount",
      };
      // No layer (or a partial one) is normal: the caller falls back to streaming
      // without an overview.
      for (const key of Object.values(k)) {
        if (!keys.has(key)) throw new Error(`net has no lines overview (${key} missing)`);
      }
      return { url, ds, k, count: (ds.shape(k.rangeStart)[0] | 0) || 0 };
    },

    linesCount(h) {
      return h.count;
    },

    // Read the whole overview — it is a few MB at most — as
    //   { positions: Float32Array(V*3), voff: Uint32Array(L+1),
    //     rangeStart, rangeCount: Uint32Array(L), count: L }
    async linesReadAll(h) {
      const { ds, k } = h;
      const voff = await ds.get(k.voff);
      const rangeStart = await ds.get(k.rangeStart);
      const rangeCount = await ds.get(k.rangeCount);
      const qlon = await ds.get(k.qlon);
      const qlat = await ds.get(k.qlat);

      // Ellipsoid: the net's own header if it has one, else the configured
      // (Mars) defaults the placeholder globe already uses.
      const num = (key, fallback) => {
        const v = parseFloat(ds.metaString(key));
        return Number.isFinite(v) && v > 0 ? v : fallback;
      };
      const a = num("h.linesRadiusA", CONFIG.placeholderRadiusA);
      const c = num("h.linesRadiusC", CONFIG.placeholderRadiusC);
      const e2 = 1 - (c * c) / (a * a);

      // Dequantize each vertex to body-fixed XYZ on the ellipsoid surface.
      const V = Math.min(qlon.length, qlat.length);
      const positions = new Float32Array(V * 3);
      for (let i = 0; i < V; i++) {
        const lon = qlon[i] / LON_SCALE;
        const lat = qlat[i] / LAT_SCALE;
        const sinLat = Math.sin(lat), cosLat = Math.cos(lat);
        const N = a / Math.sqrt(1 - e2 * sinLat * sinLat);   // prime vertical radius
        positions[i * 3] = N * cosLat * Math.cos(lon);
        positions[i * 3 + 1] = N * cosLat * Math.sin(lon);
        positions[i * 3 + 2] = N * (1 - e2) * sinLat;
      }

      return { positions, voff, rangeStart, rangeCount, count: rangeStart.length };
    },

    // Deliberately a no-op: the Dataset is shared with the point stream, which is
    // opened next and owns the release (see closePointsXYZ).
    closeLines() {},
  };
}

/**
 * Open a streaming point source over the net, PARSED BY StarDS ITSELF (its WASM
 * build, wrapped by makeNetApi). The module opens the net BY URL over /vsicurl/
 * and reads only the byte ranges it needs — opening pulls just the header/index
 * (one ranged GET); each later pointsReadXYZ pulls only the covering blocks. The
 * ~2 GB file is never downloaded whole. (No local copy is fetched here.)
 *
 * SYNCHRONOUS BY DESIGN — it returns the source object immediately and does NOT
 * await the WASM module. That is what lets initScene() start on the first frame:
 * `net` starts out null and is filled in on this same object the moment the
 * module resolves, and both opens are handed back as PROMISES. Every consumer of
 * `src.net` (the two read pumps, dispose) is gated on `pointHandle` / awaits
 * `src.ready`, and both of those are chained AFTER the module load — so nothing can
 * dereference `net` while it is still null.
 *
 * @returns {{net: any, overviewReady: Promise<{lines,summary}>, ready: Promise<{handle,total}>}}
 */
function openStream() {
  // `failed` is set if the module never loads, so boot() can retry on a later
  // document$ emit instead of leaving a permanently point-less globe.
  const src = { net: null, overviewReady: null, ready: null, failed: false };

  // The module fetch was already kicked off at module-eval time; loadStards() is
  // memoized, so this just latches onto that in-flight promise.
  const moduleReady = loadStards().then((M) => { src.net = M; return M; });
  // Marks the rejection observed (no unhandled-rejection noise — the overviewReady
  // chain below reports it) and flags the src as retryable.
  moduleReady.catch(() => { src.failed = true; });

  // Every StarDS call over /vsicurl SUSPENDS (ASYNCIFY → returns a Promise we
  // await), and ASYNCIFY allows only ONE suspension in flight — so the overview
  // open and the point-stream open must be strictly SERIAL (never overlapping).
  //
  // We hand back both as PROMISES rather than blocking here, so initScene can
  // render the PLACEHOLDER globe from the first frame (no network) while these
  // load. `overviewReady` resolves with {lines, summary}; `ready` (the point
  // stream) is CHAINED off overviewReady so its open only starts once the overview
  // open has fully completed — preserving the single-suspension invariant. Both are
  // additionally chained off `moduleReady`, so neither opens before the module
  // exists (and both reject, harmlessly, if it never loads).

  // --- Overview (preferred: polyline "lines" layer; fallback: GMM "summary"). --
  src.overviewReady = moduleReady.then(async (net) => {
    let lines = null, summary = null;
    try {
      if (typeof net.openLines === "function") {
        const lh = await net.openLines(STARDS_URL);
        const L = await net.linesCount(lh);
        if (L > 0) lines = await net.linesReadAll(lh);
        await net.closeLines(lh);
      }
    } catch (err) {
      console.warn("[StarDS hero] lines layer unavailable:", err);
      lines = null;
    }
    if (!lines) {
      try {
        if (typeof net.openSummary === "function") {
          const sh = await net.openSummary(STARDS_URL);
          const k = await net.summaryCount(sh);
          if (k > 0) summary = await net.summaryReadSplats(sh);
          await net.closeSummary(sh);
        }
      } catch (err) {
        console.warn("[StarDS hero] summary unavailable, streaming without LOD:", err);
        summary = null;
      }
    }
    return { lines, summary };
  });

  // --- Point stream: opened AFTER the overview open completes (single-suspension),
  // handed back as a promise so the pump can await it before its first read.
  src.ready = src.overviewReady.then(async () => {
    const net = src.net;   // resolved: overviewReady is chained off moduleReady
    const handle = await net.openPointsXYZ(STARDS_URL);
    const total = await net.pointsCount(handle);
    return { handle, total };
  });

  return src;
}

/**
 * Custom point shader:
 *   - circular sprite,
 *   - per-point fade-in from a birth time (streaming), and
 *   - DISTANCE FOG for the depth cue: far points blend their colour toward
 *     `uFogColor` (opacity stays uniform), so the back of the shell reads as
 *     faint/receded without the alpha-blending artefacts that per-point opacity
 *     produced. Depth is view-space z (uNearZ = front of the shell, uFarZ =
 *     back), set each frame from the camera distance and globe radius.
 */
function makePointsMaterial(pixelRatio) {
  return new THREE.ShaderMaterial({
    // OPAQUE points with real depth testing (the three.js fog model). Occlusion is
    // resolved per-pixel by the depth buffer — the nearest point at each pixel
    // wins regardless of draw order — so a back point can no longer paint over a
    // front one. Distant points are then mixed toward uFogColor; set uFogColor to
    // the page background and far points literally recede INTO the background
    // (they don't add light the way additive blending did). Front points keep
    // their full colour. See https://threejs.org/manual/#en/fog.
    transparent: false,
    depthTest: true,
    depthWrite: true,
    uniforms: {
      uTime: { value: 0 },
      uFade: { value: CONFIG.fadeDurationMs / 1000 },
      uSize: { value: CONFIG.dotSize },
      uPixelRatio: { value: pixelRatio },
      uColor: { value: new THREE.Color(CONFIG.dotColor) },
      uOpacity: { value: CONFIG.dotOpacity },
      uFogColor: { value: new THREE.Color(CONFIG.fogColor) },
      uFogStrength: { value: CONFIG.fogStrength },
      uFogFalloff: { value: CONFIG.fogFalloff },
      uNearZ: { value: 1.0 },   // view-space z of the shell front (camera side)
      uFarZ: { value: -1.0 },   // view-space z of the shell back
    },
    vertexShader: /* glsl */ `
      attribute float aBirth;          // seconds; < 0 = not yet spawned
      uniform float uTime;
      uniform float uFade;
      uniform float uSize;
      uniform float uPixelRatio;
      uniform float uNearZ;
      uniform float uFarZ;
      varying float vBorn;             // 0 = just spawned, 1 = fully faded in
      varying float vDepth;            // 0 = back of shell, 1 = front (near camera)
      void main() {
        vec4 mv = modelViewMatrix * vec4(position, 1.0);
        gl_Position = projectionMatrix * mv;
        vBorn = (aBirth < 0.0) ? 0.0 : clamp((uTime - aBirth) / uFade, 0.0, 1.0);
        // View-space z is negative in front of the camera; larger z = nearer.
        vDepth = clamp((mv.z - uFarZ) / max(uNearZ - uFarZ, 1e-3), 0.0, 1.0);
        // Unborn / not-yet-arrived points collapse to nothing; born points grow
        // in slightly as they fade, and front points render a touch larger.
        float grow = 0.55 + 0.45 * vBorn;
        float depthSize = 0.85 + 0.15 * vDepth;
        gl_PointSize = vBorn <= 0.0 ? 0.0 : uSize * uPixelRatio * grow * depthSize;
      }
    `,
    fragmentShader: /* glsl */ `
      uniform vec3 uColor;
      uniform float uOpacity;
      uniform vec3 uFogColor;
      uniform float uFogStrength;
      uniform float uFogFalloff;
      varying float vBorn;
      varying float vDepth;
      void main() {
        vec2 d = gl_PointCoord - vec2(0.5);
        if (dot(d, d) > 0.25) discard;          // clip to a circle
        if (vBorn <= 0.0) discard;              // unborn: draw nothing (no depth)
        // Everything darkens by MIXING TOWARD uFogColor (opaque, no alpha add):
        //   - distance fog: far points (vDepth→0) recede into the fog/background;
        //   - fade-in: a newborn (vBorn→0) starts at the fog colour and resolves
        //     to its true colour as it matures — a fade with no transparency;
        //   - uOpacity < 1 dims everything by holding a floor of fog mix.
        // Nearer/brighter contributions win via max(), so the front stays crisp.
        float fog = pow(1.0 - vDepth, uFogFalloff) * uFogStrength;
        float birthMix = 1.0 - vBorn;
        float dim = 1.0 - clamp(uOpacity, 0.0, 1.0);
        float m = clamp(max(max(fog, birthMix), dim), 0.0, 1.0);
        gl_FragColor = vec4(mix(uColor, uFogColor, m), 1.0);
      }
    `,
  });
}

/**
 * Lower-triangular Cholesky factor L of a symmetric 3x3 covariance given as its
 * upper triangle (xx,xy,xz,yy,yz,zz), such that Σ = L·Lᵀ. L maps a unit sphere to
 * the covariance's 1-σ ellipsoid (orientation + extent), so it is exactly the
 * per-instance transform for a splat — no eigen solver needed. Falls back to a
 * tiny isotropic scale if Σ isn't positive-definite (degenerate/empty splats).
 * Writes the 9 column-major basis entries into `out9`.
 */
function cholesky3(xx, xy, xz, yy, yz, zz, out9) {
  const l11 = Math.sqrt(Math.max(xx, 0));
  const l21 = l11 > 1e-9 ? xy / l11 : 0;
  const l31 = l11 > 1e-9 ? xz / l11 : 0;
  const l22 = Math.sqrt(Math.max(yy - l21 * l21, 0));
  const l32 = l22 > 1e-9 ? (yz - l31 * l21) / l22 : 0;
  const l33 = Math.sqrt(Math.max(zz - l31 * l31 - l32 * l32, 0));
  // Column-major 3x3 (three.js Matrix3/Matrix4 basis order): columns are the
  // images of the unit axes. L is lower-triangular: col0=(l11,l21,l31), etc.
  out9[0] = l11; out9[1] = l21; out9[2] = l31;
  out9[3] = 0;   out9[4] = l22; out9[5] = l32;
  out9[6] = 0;   out9[7] = 0;   out9[8] = l33;
}

// Deterministic per-point pseudo-random in [0,1) (mulberry32) + a Box-Muller
// standard normal, so the synthetic cloud is stable across frames/reloads.
function mulberry32(a) {
  return function () {
    a |= 0; a = (a + 0x6d2b79f5) | 0;
    let t = Math.imul(a ^ (a >>> 15), 1 | a);
    t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t;
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
}

/**
 * Build the PLACEHOLDER globe: a low-density REGULAR point grid on the Mars
 * biaxial ellipsoid, on the surface only. Shown from the first frame (it needs no
 * network) so the viewport is populated while the overview layer opens/streams,
 * then faded out once the overview is ready. Styled to match the overview (colour,
 * opacity, fog/depth cue), just sparser.
 *
 * The grid is regular in (lat,lon): `placeholderRings` latitude rings pole-to-pole,
 * and per ring the number of longitude points scales with cos(lat) so the surface
 * density stays roughly even instead of bunching at the poles. Points sit exactly
 * on the ellipsoid: x=a·cosφ·cosλ, y=a·cosφ·sinλ, z=c·sinφ (BCBF, same frame as
 * the real points / line vertices). A single `uFade` uniform (set when the overview
 * arrives) crossfades the whole grid out at once.
 * @returns {{ points: THREE.Points, mat, fadeOut(tSec) }}
 */
function makePlaceholderGlobe(pixelRatio) {
  const rings = Math.max(4, CONFIG.placeholderRings | 0);
  const A = CONFIG.placeholderRadiusA, C = CONFIG.placeholderRadiusC;
  // Longitude points on the equator ring; other rings scale by cos(lat). ~2× rings
  // gives near-square cells at the equator.
  const equatorLon = Math.max(8, Math.round(rings * 2));

  // Pass 1: build the vertex list. Rings run from just off the south pole to just
  // off the north pole (open interval) so we don't stack many points on the poles.
  const xyz = [];
  for (let r = 0; r < rings; r++) {
    const lat = -Math.PI / 2 + Math.PI * ((r + 0.5) / rings);  // (-90°,+90°) centres
    const cosLat = Math.cos(lat), sinLat = Math.sin(lat);
    const nLon = Math.max(1, Math.round(equatorLon * cosLat));
    for (let k = 0; k < nLon; k++) {
      const lon = (2 * Math.PI * k) / nLon;
      xyz.push(A * cosLat * Math.cos(lon), A * cosLat * Math.sin(lon), C * sinLat);
    }
  }
  const pos = new Float32Array(xyz);

  const geom = new THREE.BufferGeometry();
  geom.setAttribute("position", new THREE.BufferAttribute(pos, 3));

  const mat = new THREE.ShaderMaterial({
    // Same look as the line overview: additive round sprites with the distance-fog
    // depth cue. `uFade` ∈ [0,1] scales the whole grid out (1 = gone). uNearZ/uFarZ
    // set per frame from the camera, exactly like the other overlays.
    transparent: true,
    depthWrite: false,
    depthTest: false,
    blending: THREE.AdditiveBlending,
    uniforms: {
      uColor: { value: new THREE.Color(CONFIG.placeholderColor) },
      uOpacity: { value: CONFIG.placeholderOpacity },
      uFade: { value: 0 },     // 0 = fully visible, 1 = faded out
      uSize: { value: CONFIG.placeholderDotSize },
      uPixelRatio: { value: pixelRatio },
      uFogColor: { value: new THREE.Color(CONFIG.fogColor) },
      uFogStrength: { value: CONFIG.fogStrength },
      uFogFalloff: { value: CONFIG.fogFalloff },
      uNearZ: { value: 1.0 },
      uFarZ: { value: -1.0 },
    },
    vertexShader: /* glsl */ `
      uniform float uSize, uPixelRatio, uNearZ, uFarZ, uFade;
      varying float vDepth;
      void main() {
        vec4 mv = modelViewMatrix * vec4(position, 1.0);
        gl_Position = projectionMatrix * mv;
        vDepth = clamp((mv.z - uFarZ) / max(uNearZ - uFarZ, 1e-3), 0.0, 1.0);
        float depthSize = 0.85 + 0.15 * vDepth;
        gl_PointSize = uFade >= 1.0 ? 0.0 : uSize * uPixelRatio * depthSize;
      }
    `,
    fragmentShader: /* glsl */ `
      uniform vec3 uColor, uFogColor;
      uniform float uOpacity, uFogStrength, uFogFalloff, uFade;
      varying float vDepth;
      void main() {
        vec2 d = gl_PointCoord - vec2(0.5);
        if (dot(d, d) > 0.25) discard;          // round sprite
        float vis = 1.0 - uFade;
        if (vis <= 0.0) discard;
        float fog = pow(1.0 - vDepth, uFogFalloff) * uFogStrength;
        vec3 rgb = mix(uColor, uFogColor, fog);
        float bright = uOpacity * vis * (1.0 - 0.85 * fog);
        if (bright <= 0.0) discard;
        // ALPHA MUST TRACK BRIGHTNESS. The canvas is transparent (clearAlpha 0) over
        // the CSS gradient, and additive blending adds alpha as well as colour — so a
        // hard-coded 1.0 makes every covered pixel fully opaque even when the colour
        // contribution is ~0, painting a black disc over the gradient. Premultiplied
        // (rgb*bright, bright) keeps colour identical and leaves dim points sheer.
        gl_FragColor = vec4(rgb * bright, bright);
      }
    `,
  });

  const points = new THREE.Points(geom, mat);
  points.frustumCulled = false;

  // Crossfade the whole grid out starting at tSeconds; the tick loop advances
  // uFade from the stored start using uTime. We store the start on the material.
  let fadeStart = -1;
  function fadeOut(tSeconds) { if (fadeStart < 0) fadeStart = tSeconds; }
  // Expose the fade start so the tick loop can ramp uFade over placeholderFadeOutMs.
  points.userData.getFadeStart = () => fadeStart;

  return { points, mat, fadeOut };
}

/**
 * Build the polyline overview as POINTS sampled along each line (disc sprites,
 * like the real cloud) — so the overview reads as a point cloud, not wireframe.
 * Each line is sampled by arc length at ~`lineSampleSpacing` metres (at least its
 * own vertices, so short lines still show). Points carry a per-point `aArrival`
 * for the per-line crossfade, and the shader applies the same fog/depth cue as
 * the real points. Sampled points are laid out in contiguous per-line blocks so
 * fade-out is an O(block) stamp.
 * @param {{positions:Float32Array, voff:Uint32Array, count:number}} lines
 * @returns {{ points: THREE.Points, mat, markLineArrived(i,tSec) }}
 */
function makeLineMesh(lines, pixelRatio) {
  const L = lines.count | 0;
  const V = lines.positions;          // V*3 floats, all lines' vertices in order
  const voff = lines.voff;            // vertex CSR (L+1)
  const spacing = Math.max(1, CONFIG.lineSampleSpacing);

  // Pass 1: count samples per line = ceil(arcLength/spacing)+1, clamped so the
  // total respects lineMaxSamples (scale spacing up if we'd blow the budget).
  const segLen = (a, b) => {
    const dx = V[b*3]-V[a*3], dy = V[b*3+1]-V[a*3+1], dz = V[b*3+2]-V[a*3+2];
    return Math.sqrt(dx*dx + dy*dy + dz*dz);
  };
  const lineArc = new Float64Array(L);
  let totalArc = 0;
  for (let i = 0; i < L; i++) {
    let arc = 0;
    for (let v = voff[i]; v + 1 < voff[i+1]; v++) arc += segLen(v, v+1);
    lineArc[i] = arc; totalArc += arc;
  }
  // Effective spacing: never finer than `spacing`, but coarsen if the budget
  // would be exceeded (≈ totalArc/budget).
  const budget = Math.max(1, CONFIG.lineMaxSamples | 0);
  const eff = Math.max(spacing, totalArc / budget);

  const start = new Uint32Array(L);
  const count = new Uint32Array(L);
  let total = 0;
  for (let i = 0; i < L; i++) {
    const nv = voff[i+1] - voff[i];
    let ns = nv <= 1 ? nv : Math.max(nv, Math.floor(lineArc[i] / eff) + 1);
    start[i] = total; count[i] = ns; total += ns;
  }

  // Pass 2: sample each line at equal arc-length steps into the point buffer.
  const pos = new Float32Array(total * 3);
  const arr = new Float32Array(total).fill(-1);   // per-point arrival time
  for (let i = 0; i < L; i++) {
    const a = voff[i], b = voff[i+1], ns = count[i], w0 = start[i];
    const nv = b - a;
    if (nv === 0) continue;
    if (nv === 1 || ns <= 1) {
      pos[w0*3] = V[a*3]; pos[w0*3+1] = V[a*3+1]; pos[w0*3+2] = V[a*3+2];
      continue;
    }
    // Walk the polyline placing ns points at arc positions [0..arc] evenly.
    const arc = lineArc[i], stepArc = arc / (ns - 1);
    let seg = a, segStartArc = 0, segL = segLen(a, a+1);
    for (let s = 0; s < ns; s++) {
      let target = s * stepArc;
      // advance to the segment containing `target`
      while (seg + 2 < b && target > segStartArc + segL) {
        segStartArc += segL; seg++; segL = segLen(seg, seg+1);
      }
      const t = segL > 0 ? Math.min(1, Math.max(0, (target - segStartArc) / segL)) : 0;
      const w = w0 + s;
      pos[w*3]   = V[seg*3]   + t * (V[(seg+1)*3]   - V[seg*3]);
      pos[w*3+1] = V[seg*3+1] + t * (V[(seg+1)*3+1] - V[seg*3+1]);
      pos[w*3+2] = V[seg*3+2] + t * (V[(seg+1)*3+2] - V[seg*3+2]);
    }
  }

  const geom = new THREE.BufferGeometry();
  geom.setAttribute("position", new THREE.BufferAttribute(pos, 3));
  const aArrival = new THREE.BufferAttribute(arr, 1).setUsage(THREE.DynamicDrawUsage);
  geom.setAttribute("aArrival", aArrival);

  const mat = new THREE.ShaderMaterial({
    // Points (disc sprites), transparent for the crossfade, with the SAME distance
    // fog + depth cue as the real cloud: far samples blend toward uFogColor and
    // dim so the back recedes into the background. uNearZ/uFarZ set per frame.
    transparent: true,
    depthWrite: false,
    depthTest: false,
    blending: THREE.AdditiveBlending,
    uniforms: {
      uTime: { value: 0 },
      uColor: { value: new THREE.Color(CONFIG.lineColor) },
      uOpacity: { value: CONFIG.lineOpacity },
      uFadeOut: { value: CONFIG.lineFadeOutMs / 1000 },
      // 1 = ignore the arrival fade and show every line, even ones whose real points
      // already landed. Set while the point cloud isn't being drawn (see
      // applyLayerOpacity) — there's nothing to hand off to, so nothing should hide.
      uShowArrived: { value: 0 },
      uSize: { value: CONFIG.lineDotSize },
      uPixelRatio: { value: pixelRatio },
      uFogColor: { value: new THREE.Color(CONFIG.fogColor) },
      uFogStrength: { value: CONFIG.fogStrength },
      uFogFalloff: { value: CONFIG.fogFalloff },
      uNearZ: { value: 1.0 },
      uFarZ: { value: -1.0 },
    },
    vertexShader: /* glsl */ `
      attribute float aArrival;
      uniform float uTime, uFadeOut, uSize, uPixelRatio, uNearZ, uFarZ, uShowArrived;
      varying float vVis;
      varying float vDepth;            // 0 = back of shell, 1 = front (near camera)
      void main() {
        vec4 mv = modelViewMatrix * vec4(position, 1.0);
        gl_Position = projectionMatrix * mv;
        float vis = 1.0;
        if (aArrival >= 0.0 && uShowArrived < 0.5)
          vis = 1.0 - clamp((uTime - aArrival) / uFadeOut, 0.0, 1.0);
        vVis = vis;
        vDepth = clamp((mv.z - uFarZ) / max(uNearZ - uFarZ, 1e-3), 0.0, 1.0);
        float depthSize = 0.85 + 0.15 * vDepth;   // front points a touch larger
        gl_PointSize = vis <= 0.0 ? 0.0 : uSize * uPixelRatio * depthSize;
      }
    `,
    fragmentShader: /* glsl */ `
      uniform vec3 uColor, uFogColor;
      uniform float uOpacity, uFogStrength, uFogFalloff;
      varying float vVis;
      varying float vDepth;
      void main() {
        vec2 d = gl_PointCoord - vec2(0.5);
        if (dot(d, d) > 0.25) discard;          // round sprite
        if (vVis <= 0.0) discard;
        // Distance fog: far (vDepth→0) samples recede toward the fog/background
        // colour AND lose brightness, matching the point shader's depth cue.
        float fog = pow(1.0 - vDepth, uFogFalloff) * uFogStrength;
        vec3 rgb = mix(uColor, uFogColor, fog);
        float bright = uOpacity * vVis * (1.0 - 0.85 * fog);   // back dims out
        if (bright <= 0.0) discard;
        // Premultiplied, with alpha = brightness: additive blending adds alpha too, so
        // writing 1.0 here turned a dimmed (or slider-faded) overview into an opaque
        // black shell over the page's gradient instead of making it disappear.
        gl_FragColor = vec4(rgb * bright, bright);
      }
    `,
  });

  const points = new THREE.Points(geom, mat);
  points.frustumCulled = false;

  function markLineArrived(i, tSeconds) {
    const s = start[i], c = count[i];
    if (c === 0 || arr[s] >= 0) return;
    for (let j = 0; j < c; j++) arr[s + j] = tSeconds;
    aArrival.addUpdateRange(s, c);
    aArrival.needsUpdate = true;
  }

  return { points, mat, markLineArrived };
}

/**
 * RECONSTRUCT the point cloud from the GMM: sample `gmmPoints` synthetic points
 * from the mixture — pick a splat with probability ∝ weight, then draw
 * p = μ + L·z (z ~ N(0,I)³, L = Cholesky(Σ)). This regenerates the tracks from
 * only the ~few-hundred-KB model, no real points fetched. Each synthetic point
 * also carries the index of the splat it came from (`aSplat`), so it can be
 * faded out per-splat once that splat's real points stream in.
 *
 * Returns { points: THREE.Points, splatArrival: Float32Array(K) } — the mesh and
 * the per-splat "real points arrived at t" array the shader reads for fade-out.
 * @param {{muX,muY,muZ,s0..s5,weight,rangeStart,rangeCount,count}} splats
 */
function makeGmmCloud(splats, pixelRatio) {
  const K = splats.count | 0;
  const N = Math.max(0, CONFIG.gmmPoints | 0);

  // Alias-free weighted pick via a CDF over splat weights (counts).
  const cdf = new Float64Array(K);
  let total = 0;
  for (let i = 0; i < K; i++) { total += splats.weight[i] || 0; cdf[i] = total; }
  if (total <= 0) return null;

  // Allot each splat a share of N proportional to its weight, laid out in
  // CONTIGUOUS blocks by splat index. That makes per-splat fade-out an O(block)
  // range stamp (not an O(N) scan) when a splat's real points arrive, and it lets
  // the sampler write points grouped by splat. gmmStart[k]..gmmStart[k]+gmmCount[k].
  const gmmStart = new Uint32Array(K);
  const gmmCount = new Uint32Array(K);
  {
    let acc = 0, assigned = 0;
    for (let k = 0; k < K; k++) {
      acc += splats.weight[k] || 0;
      const upto = Math.round((acc / total) * N);
      gmmStart[k] = assigned;
      gmmCount[k] = Math.max(0, upto - assigned);
      assigned = upto;
    }
  }

  const positions = new Float32Array(N * 3);
  const rand = mulberry32(0x9e3779b9);
  const L = new Float64Array(9);
  const gauss = () => {
    let u = 0, v = 0;
    while (u === 0) u = rand();
    while (v === 0) v = rand();
    return Math.sqrt(-2 * Math.log(u)) * Math.cos(2 * Math.PI * v);
  };

  // Sample each splat's block: p = μ + L·z (L column-major col j=(L[3j..3j+2])).
  for (let k = 0; k < K; k++) {
    cholesky3(splats.s0[k], splats.s1[k], splats.s2[k],
              splats.s3[k], splats.s4[k], splats.s5[k], L);
    const start = gmmStart[k], cnt = gmmCount[k];
    const mx = splats.muX[k], my = splats.muY[k], mz = splats.muZ[k];
    for (let j = 0; j < cnt; j++) {
      const z0 = gauss(), z1 = gauss(), z2 = gauss();
      const n = (start + j) * 3;
      positions[n]     = mx + L[0] * z0 + L[3] * z1 + L[6] * z2;
      positions[n + 1] = my + L[1] * z0 + L[4] * z1 + L[7] * z2;
      positions[n + 2] = mz + L[2] * z0 + L[5] * z1 + L[8] * z2;
    }
  }

  // Per-point arrival time (seconds); -1 = this splat's real points not loaded.
  // The shader fades a synthetic point out over gmmFadeOutMs from its arrival.
  const arrivalAttr = new Float32Array(N).fill(-1);

  const geom = new THREE.BufferGeometry();
  geom.setAttribute("position", new THREE.BufferAttribute(positions, 3));
  const aArrival = new THREE.BufferAttribute(arrivalAttr, 1).setUsage(THREE.DynamicDrawUsage);
  geom.setAttribute("aArrival", aArrival);

  const mat = new THREE.ShaderMaterial({
    transparent: true,
    depthWrite: false,
    depthTest: false,
    blending: THREE.AdditiveBlending,
    uniforms: {
      uTime: { value: 0 },
      uSize: { value: CONFIG.dotSize },
      uPixelRatio: { value: pixelRatio },
      uColor: { value: new THREE.Color(CONFIG.gmmColor) },
      uOpacity: { value: CONFIG.gmmOpacity },
      uFadeOut: { value: CONFIG.gmmFadeOutMs / 1000 },
      uShowArrived: { value: 0 },   // 1 = keep faded-out splats on screen (see the lines shader)
    },
    vertexShader: /* glsl */ `
      attribute float aArrival;             // seconds real points arrived; <0 = not yet
      uniform float uTime, uSize, uPixelRatio, uFadeOut, uShowArrived;
      varying float vVis;
      void main() {
        gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
        // Full brightness until this splat's real points arrive, then fade out.
        float vis = 1.0;
        if (aArrival >= 0.0 && uShowArrived < 0.5)
          vis = 1.0 - clamp((uTime - aArrival) / uFadeOut, 0.0, 1.0);
        vVis = vis;
        gl_PointSize = vis <= 0.0 ? 0.0 : uSize * uPixelRatio;
      }
    `,
    fragmentShader: /* glsl */ `
      uniform vec3 uColor; uniform float uOpacity;
      varying float vVis;
      void main() {
        vec2 d = gl_PointCoord - vec2(0.5);
        if (dot(d, d) > 0.25) discard;      // round sprite
        if (vVis <= 0.0) discard;
        float bright = uOpacity * vVis;
        if (bright <= 0.0) discard;
        // Premultiplied, alpha = brightness (see the lines shader): additive blending
        // adds alpha, so a fixed 1.0 would darken the gradient wherever a splat is dim.
        gl_FragColor = vec4(uColor * bright, bright);
      }
    `,
  });

  const points = new THREE.Points(geom, mat);
  points.frustumCulled = false;

  // Called when splat `k`'s real points arrive: stamp its contiguous synthetic
  // block with the arrival time so the shader fades exactly those points out.
  // O(block) per splat, each splat once → O(N) total across the run.
  function markSplatArrived(k, tSeconds) {
    const start = gmmStart[k], cnt = gmmCount[k];
    if (cnt === 0 || arrivalAttr[start] >= 0) return;
    for (let j = 0; j < cnt; j++) arrivalAttr[start + j] = tSeconds;
    aArrival.addUpdateRange(start, cnt);
    aArrival.needsUpdate = true;
  }

  return { points, mat, markSplatArrived };
}

/**
 * Initialize the Three.js scene inside `canvas`, sized to `host`, and stream the
 * point cloud in from `src` over time.
 *
 * Called as soon as Three.js is available — the WASM module is typically STILL
 * LOADING at this point, so `src.net` may be null on entry and is filled in
 * later (see openStream). It is only ever read once `pointHandle` is set or from
 * inside a `src.ready` continuation, both of which imply the module has resolved.
 *
 * @param {HTMLCanvasElement} canvas
 * @param {HTMLElement} host
 * @param {{net: any, overviewReady: Promise<{lines,summary}>, ready: Promise<{handle,total}>}} src
 */
function initScene(canvas, host, src) {
  const renderer = new THREE.WebGLRenderer({ canvas, alpha: true, antialias: true });
  const pixelRatio = Math.min(window.devicePixelRatio || 1, 2);
  renderer.setPixelRatio(pixelRatio);
  renderer.setClearAlpha(CONFIG.clearAlpha);

  const scene = new THREE.Scene();

  // Both the overview and the point-stream handle open in the BACKGROUND
  // (src.overviewReady, src.ready). Until the overview resolves we render only the
  // placeholder globe; until the point handle lands the pump no-ops. Because
  // src.ready is chained AFTER src.overviewReady, `pointHandle` can't be set before
  // the overview overlay is built — so the pumps never run without an overview in
  // place. We don't know the overview's point-coverage bound yet, so size the
  // streaming buffers to the hard cap (always an upper bound on
  // target = min(total, maxPoints)); `target` is tightened once the overview lands.
  let pointHandle = null;
  let target = CONFIG.maxPoints;
  // Highest point index the overview's windows reach (its coverage bound). Set when
  // the overview lands; the "load all" button can't ask for more than this, since
  // points past it are never referenced by any window.
  let coveredPoints = Infinity;

  // Preallocate the GPU buffers; batches fill regions of them over time. Not const:
  // the "load all points" button lifts the cap, which regrows them (growPointBuffers).
  let positions = new Float32Array(target * 3);
  let births = new Float32Array(target).fill(-1); // -1 = not yet spawned
  let posAttr = new THREE.BufferAttribute(positions, 3).setUsage(THREE.DynamicDrawUsage);
  let birthAttr = new THREE.BufferAttribute(births, 1).setUsage(THREE.DynamicDrawUsage);

  const geom = new THREE.BufferGeometry();
  geom.setAttribute("position", posAttr);
  geom.setAttribute("aBirth", birthAttr);
  geom.setDrawRange(0, 0); // nothing loaded yet

  const material = makePointsMaterial(pixelRatio);

  const cloud = new THREE.Points(geom, material);
  // Disable frustum culling. The geometry streams in over time, so at the first
  // render the position buffer is still all-zeros and three.js would compute a
  // radius-0 bounding sphere at the origin — then never recompute it, culling the
  // whole (multi-million-metre) cloud once real points arrive. The cloud always
  // fills the view when present, so skip culling entirely.
  cloud.frustumCulled = false;

  // Scene graph for a tilted, pole-up globe:
  //   orient (stands the pole up + leans it by axisTilt)
  //     └─ spin (rotates the body about its pole over time)
  //          └─ cloud
  // The body's poles are on its Z axis, so `spin` rotates about Z; `orient` first
  // maps Z→up (screen +Y) then applies the tilt, so the whole thing turns on a
  // slightly leaned axis like Mars instead of spinning with the poles on the side.
  const spin = new THREE.Group();
  spin.add(cloud);
  const orient = new THREE.Group();
  orient.add(spin);
  // userOrbit sits ABOVE orient so the trackpad can orbit the whole globe (yaw +
  // pitch) on top of the fixed pole-up tilt and the time-based auto-spin. Keeping
  // it outside `orient`/`spin` means hand-orbit never disturbs the spin that
  // paces streaming, and the camera stays put (framing/right-shift preserved).
  const userOrbit = new THREE.Group();
  userOrbit.add(orient);
  scene.add(userOrbit);
  // Hand-orbit is stored as a QUATERNION accumulated incrementally about the world
  // screen axes (trackball) — NOT Euler yaw/pitch. This is gimbal-lock free: there
  // is no pole singularity and no axis can collapse onto another, so the globe can
  // tumble to any orientation and keep dragging smoothly. (Reusable temps below
  // avoid per-move allocation.)
  const orbitQuat = new THREE.Quaternion();
  const _dq = new THREE.Quaternion();
  const _axisX = new THREE.Vector3(1, 0, 0);   // screen right
  const _axisY = new THREE.Vector3(0, 1, 0);   // screen up

  // PLACEHOLDER globe — a low-density regular grid on the Mars ellipsoid, drawn
  // from the FIRST frame (no network) so the viewport is never empty while the
  // overview layer opens/streams. Added to `spin` so it rotates with the globe.
  // It fades out the instant the overview overlay is built (see overviewReady).
  let placeholder = null;   // { points, mat, fadeOut }
  if (CONFIG.showPlaceholder) {
    placeholder = makePlaceholderGlobe(pixelRatio);
    placeholder.points.renderOrder = -2;   // behind the overview and real points
    spin.add(placeholder.points);
  }

  // LOD-0 overview, added to the `spin` group so it rotates with the globe. Built
  // LATER, when src.overviewReady resolves (see below) — until then the placeholder
  // stands in. Preferred: the polyline ("lines") model — filaments drawn as
  // ribbons, each fading out as its real points stream in. Fallback: the GMM
  // synthetic cloud. Both present only if the corresponding layer exists.
  let overview = null;      // the overview data ({lines}|{summary}) once resolved
  let overviewSettled = false;   // true once overviewReady resolves (present or not)
  let hasLines = false, hasSummary = false;
  let lineOverlay = null;   // { points, mat, markLineArrived }
  let gmm = null;           // { points, mat, markSplatArrived }

  /** Point the spin axis (body Z) up and lean it by CONFIG.axisTilt. Rebuilt from
   *  CONFIG so the tilt is tunable live via msHero.apply(). */
  function applyOrientation() {
    const tilt = THREE.MathUtils.degToRad(CONFIG.axisTilt || 0);
    const roll = THREE.MathUtils.degToRad(CONFIG.axisTiltRoll || 0);
    // Base: rotate -90° about X so the body's +Z pole points to screen +Y (up).
    // Tilt: lean that up-axis by `tilt`; `roll` spins the lean around vertical so
    // it can tip back/front (roll 0) or left/right (roll 90).
    const e = new THREE.Euler(-Math.PI / 2 + tilt, 0, 0, "XYZ");
    orient.quaternion.setFromEuler(e);
    if (roll) {
      const rollQ = new THREE.Quaternion().setFromAxisAngle(new THREE.Vector3(0, 1, 0), roll);
      orient.quaternion.premultiply(rollQ);
    }
  }
  applyOrientation();

  // Body-centered coords: the globe is centered on the origin, so frame from the
  // body radius. Seeded with a Mars-ish radius; refined from the first batch.
  let radius = 3.4e6;
  const camera = new THREE.PerspectiveCamera(45, 1, radius * 0.01, radius * 100);
  let framedFromData = false;

  /** Mask the canvas so points only start rendering past a vertical line from the
   *  left (keeps the headline area clear). `renderEdge` is where the cloud begins
   *  to appear; it ramps to fully visible over `renderEdgeSoftness`. Driven from
   *  CONFIG here (not static CSS) so both are tunable live via msHero.apply(). */
  function applyRenderEdge() {
    const a = Math.max(0, Math.min(1, CONFIG.renderEdge)) * 100;
    const b = Math.min(100, a + Math.max(0, CONFIG.renderEdgeSoftness) * 100);
    const grad = `linear-gradient(to right, transparent 0%, transparent ${a.toFixed(1)}%, #000 ${b.toFixed(1)}%)`;
    canvas.style.webkitMaskImage = grad;
    canvas.style.maskImage = grad;
  }
  applyRenderEdge();

  /** Place the camera: distance from `zoom`, and shift the globe toward the
   *  right of the frame via the projection's view offset. */
  function frame() {
    const w = host.clientWidth || 1;
    const h = host.clientHeight || 1;
    renderer.setSize(w, h, false);
    camera.aspect = w / h;
    camera.near = radius * 0.01;
    camera.far = radius * 100;

    // Distance so the globe's bounding sphere fits, then divide by zoom to push in.
    const fitDist = radius / Math.tan((camera.fov * Math.PI) / 180 / 2);
    camera.position.set(0, 0, fitDist / CONFIG.zoom);
    camera.lookAt(0, 0, 0);

    // setViewOffset renders a shifted sub-window of the full frame, sliding the
    // (centered) globe toward the right / vertically without moving the camera
    // off-axis. Offsets are in pixels of the full frame.
    const dx = -CONFIG.offsetX * w;   // negative full-x => content moves right
    const dy = -CONFIG.offsetY * h;
    camera.setViewOffset(w, h, dx, dy, w, h);
    camera.updateProjectionMatrix();

    // Depth-contrast bounds: the cloud is a shell of ~`radius` centered at the
    // origin, camera at distance D on +Z. View-space z spans [-radius-D (back),
    // radius-D (front)]; the shaders ramp the fog/depth cue between these. The
    // line overlay uses the same bounds so its fog matches the points.
    const D = camera.position.z;
    material.uniforms.uNearZ.value = radius - D;
    material.uniforms.uFarZ.value = -radius - D;
    // Overlays share the same depth bounds so their fog matches the points. Each
    // may be null (not yet built / already faded), so guard.
    if (lineOverlay) {
      lineOverlay.mat.uniforms.uNearZ.value = radius - D;
      lineOverlay.mat.uniforms.uFarZ.value = -radius - D;
    }
    if (placeholder) {
      placeholder.mat.uniforms.uNearZ.value = radius - D;
      placeholder.mat.uniforms.uFarZ.value = -radius - D;
    }
  }
  frame();

  const ro = new ResizeObserver(frame);
  ro.observe(host);

  // --- Subtle "N points loaded" counter --------------------------------------
  // Created in JS so the markup stays clean; styled via .ms-hero__counter.
  const counterEl = document.createElement("div");
  counterEl.className = "ms-hero__counter";
  counterEl.setAttribute("aria-hidden", "true");
  host.appendChild(counterEl);

  // Static caption above the counter, sharing its exact styling (see the shared
  // .ms-hero__counter, .ms-hero__caption rule in extra.css).
  const captionEl = document.createElement("div");
  captionEl.className = "ms-hero__caption";
  captionEl.setAttribute("aria-hidden", "true");
  captionEl.textContent =
    "Real-time streaming of an MRO CTX control network from a .stards file over /vsicurl/";
  host.appendChild(captionEl);

  // --- Controls toggle (top-right) -------------------------------------------
  // The interactive controls (zoom, orbit pad, opacity) stay out of the way until
  // asked for: this button flips `is-controls-open` on the hero, and CSS reveals
  // the whole group off that one class (see .ms-hero__toggle / the
  // .ms-hero.is-controls-open rules in extra.css). The button itself is the only
  // control visible at rest, so a first-time visitor sees an uncluttered globe.
  const toggleEl = document.createElement("button");
  toggleEl.type = "button";
  toggleEl.className = "ms-hero__toggle";
  toggleEl.title = "View controls";
  // Three horizontal sliders — the conventional "adjust the view" affordance.
  toggleEl.innerHTML =
    '<svg viewBox="0 0 24 24" aria-hidden="true" focusable="false">' +
    '<path d="M4 7h10M18 7h2M4 12h3M11 12h9M4 17h9M17 17h3" ' +
    'fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round"/>' +
    '<circle cx="16" cy="7" r="2" fill="currentColor"/>' +
    '<circle cx="9" cy="12" r="2" fill="currentColor"/>' +
    '<circle cx="15" cy="17" r="2" fill="currentColor"/></svg>';
  host.appendChild(toggleEl);

  let controlsOpen = false;
  function setControlsOpen(open) {
    controlsOpen = open;
    host.classList.toggle("is-controls-open", open);
    toggleEl.setAttribute("aria-expanded", String(open));
    toggleEl.setAttribute("aria-label", open ? "Hide view controls" : "Show view controls");
  }
  const onToggle = () => setControlsOpen(!controlsOpen);
  toggleEl.addEventListener("click", onToggle);
  setControlsOpen(false);

  // --- Subtle zoom slider (right edge) ---------------------------------------
  // A native range input (styled + rotated to vertical in extra.css) that drives
  // CONFIG.zoom live via frame(). Dragging up zooms IN. Bounds bracket the default
  // (CONFIG.zoom = 1.5): out to ~0.6 (whole globe with margin), in to ~4.
  const ZOOM_MIN = 0.6, ZOOM_MAX = 4.0;
  const zoomWrap = document.createElement("div");
  zoomWrap.className = "ms-hero__zoom";
  const zoomInput = document.createElement("input");
  zoomInput.type = "range";
  zoomInput.min = String(ZOOM_MIN);
  zoomInput.max = String(ZOOM_MAX);
  zoomInput.step = "0.01";
  zoomInput.value = String(Math.min(ZOOM_MAX, Math.max(ZOOM_MIN, CONFIG.zoom)));
  zoomInput.setAttribute("aria-label", "Zoom the globe");
  zoomInput.setAttribute("title", "Zoom");
  const onZoom = () => {
    CONFIG.zoom = parseFloat(zoomInput.value) || CONFIG.zoom;
    frame();
  };
  zoomInput.addEventListener("input", onZoom);
  zoomWrap.appendChild(zoomInput);
  host.appendChild(zoomWrap);

  // --- Virtual orbit trackpad (bottom-right) ---------------------------------
  // A small pad you drag on to spin the globe by hand. Dragging maps the pointer's
  // MOVEMENT directly to rotation — Δx → yaw, Δy → pitch, scaled so one pad-width
  // of travel ≈ trackpadSensitivity radians — so it reads like pushing the sphere
  // with a finger. It rotates ONLY while the pointer is held down inside the pad
  // (pointer capture keeps the drag alive if you slide off the edge). Pointer
  // Events cover mouse + touch + pen with one code path. Accumulates into the
  // orbitQuat trackball quaternion (gimbal-lock free) — see onPadMove.
  let padActive = false;          // a drag is in progress
  let padLastX = 0, padLastY = 0; // previous pointer position (client px)
  const padWrap = document.createElement("div");
  padWrap.className = "ms-hero__pad";
  padWrap.setAttribute("aria-hidden", "true");
  // The pad face is a PS5-style dot-matrix, drawn purely in CSS (see .ms-hero__pad
  // ::before in extra.css) — no label element needed.
  host.appendChild(padWrap);
  if (!CONFIG.trackpad) padWrap.style.display = "none";

  const onPadDown = (e) => {
    padActive = true;
    padLastX = e.clientX; padLastY = e.clientY;
    padWrap.classList.add("is-active");
    try { padWrap.setPointerCapture(e.pointerId); } catch (_) {}
    e.preventDefault();
  };
  const onPadMove = (e) => {
    if (!padActive) return;
    // Scale by pad width so sensitivity is resolution-independent: dragging across
    // the whole pad turns the globe by ~trackpadSensitivity radians.
    const w = padWrap.getBoundingClientRect().width || 1;
    const k = CONFIG.trackpadSensitivity / w;
    const dYaw = (e.clientX - padLastX) * k;         // drag right → globe yaws right
    const dPitch = (e.clientY - padLastY) * k;       // drag down → top tips toward you
    // Compose incremental rotations about the WORLD screen axes and PRE-multiply
    // the accumulated orbit quaternion (world-space trackball). Using fixed world
    // axes — not the globe's current local axes — keeps drag direction intuitive
    // regardless of orientation, and quaternions have no gimbal lock, so there's
    // no pitch clamp needed: the globe can tumble fully over the poles.
    _dq.setFromAxisAngle(_axisY, dYaw);
    orbitQuat.premultiply(_dq);
    _dq.setFromAxisAngle(_axisX, dPitch);
    orbitQuat.premultiply(_dq);
    orbitQuat.normalize();                           // guard against drift over many moves
    padLastX = e.clientX; padLastY = e.clientY;
    e.preventDefault();
  };
  const onPadUp = (e) => {
    if (!padActive) return;
    padActive = false;
    padWrap.classList.remove("is-active");
    try { padWrap.releasePointerCapture(e.pointerId); } catch (_) {}
  };
  padWrap.addEventListener("pointerdown", onPadDown);
  padWrap.addEventListener("pointermove", onPadMove);
  padWrap.addEventListener("pointerup", onPadUp);
  padWrap.addEventListener("pointercancel", onPadUp);
  // Never scroll/gesture the page while dragging on touch.
  padWrap.style.touchAction = "none";

  // --- Layer opacity sliders (top-right, under the toggle) --------------------
  // Two horizontal sliders fading the two data layers independently:
  //
  //   "Points"   -> CONFIG.dotOpacity, the streamed point cloud.
  //   "Overview" -> CONFIG.lineOpacity + CONFIG.gmmOpacity, the coarse layer that
  //                 stands in for the cloud while it loads (sampled line points and
  //                 the GMM splats are one visual layer, so one slider drives both;
  //                 the slider carries the line opacity and the GMM tracks it at the
  //                 ratio of their tuned defaults, so their relative weighting holds).
  //
  // Each slider is the layer's opacity outright, 0..1, and starts at the value tuned
  // in CONFIG — so a layer that ships at less than full opacity can be pushed up as
  // well as down, rather than starting pinned at the top of its range.
  //
  // Both write CONFIG and call applyLayerOpacity(), which pokes the live shader
  // uniforms directly — the same values msHero.apply() writes, so console tweaks
  // and the sliders can't fight each other.
  const GMM_OPACITY_RATIO = CONFIG.lineOpacity ? CONFIG.gmmOpacity / CONFIG.lineOpacity : 1;
  const clamp01 = (v) => Math.min(1, Math.max(0, v));

  // The point cloud is OPAQUE (depth-tested, so the front of the globe can't be
  // painted over by its back), so it has no alpha to turn down: its uOpacity dims by
  // mixing toward uFogColor. That leaves a flat fog-coloured ghost of the cloud at
  // opacity 0 — visible against the hero's GRADIENT backdrop, and thick enough to
  // hide the overview behind it. So at 0 we stop DRAWING it entirely (visible =
  // false). The buffers, births and stream all stay exactly as they are — nothing is
  // freed or re-fetched, and raising the slider shows the same points instantly.
  //
  // While the cloud is hidden, the overview also suspends its per-line arrival
  // fade-out (uShowArrived): that crossfade only makes sense as a handoff TO the real
  // points, so with them not drawn the overview would otherwise be invisible too —
  // every line it already handed off to has faded, which is the whole layer once the
  // full network is loaded.
  function applyLayerOpacity() {
    material.uniforms.uOpacity.value = CONFIG.dotOpacity;
    const showPoints = CONFIG.dotOpacity > 0;
    cloud.visible = showPoints;
    if (lineOverlay) {
      lineOverlay.mat.uniforms.uOpacity.value = CONFIG.lineOpacity;
      lineOverlay.mat.uniforms.uShowArrived.value = showPoints ? 0 : 1;
    }
    if (gmm) {
      gmm.mat.uniforms.uOpacity.value = CONFIG.gmmOpacity;
      gmm.mat.uniforms.uShowArrived.value = showPoints ? 0 : 1;
    }
  }

  const opacityWrap = document.createElement("div");
  opacityWrap.className = "ms-hero__opacity";

  function makeOpacitySlider(label, initial, onValue) {
    const row = document.createElement("label");
    row.className = "ms-hero__opacity-row";
    const text = document.createElement("span");
    text.className = "ms-hero__opacity-label";
    text.textContent = label;
    const input = document.createElement("input");
    input.type = "range";
    input.min = "0";
    input.max = "1";
    input.step = "0.01";
    input.value = String(initial);
    input.setAttribute("aria-label", `${label} layer opacity`);
    const handler = () => {
      onValue(parseFloat(input.value));
      applyLayerOpacity();
      frame();
    };
    input.addEventListener("input", handler);
    row.appendChild(text);
    row.appendChild(input);
    opacityWrap.appendChild(row);
    return { input, handler };
  }

  const dotOpacityCtl = makeOpacitySlider("Points", clamp01(CONFIG.dotOpacity), (v) => {
    CONFIG.dotOpacity = v;
  });
  const overviewOpacityCtl = makeOpacitySlider("Overview", clamp01(CONFIG.lineOpacity), (v) => {
    CONFIG.lineOpacity = v;
    CONFIG.gmmOpacity = clamp01(v * GMM_OPACITY_RATIO);
  });
  host.appendChild(opacityWrap);

  const numFmt = new Intl.NumberFormat();
  // Sample the load rate on a fixed interval and smooth it (EMA) so the readout
  // shows a steady points/second rather than per-frame jitter.
  let rate = 0;               // smoothed points/second
  let rateSampleAt = 0;       // ms of the last rate sample
  let rateSampleLoaded = 0;   // `loaded` at the last rate sample
  let lastText = "";
  function updateCounter(nowMs) {
    if (rateSampleAt === 0) { rateSampleAt = nowMs; rateSampleLoaded = loaded; }
    const dt = nowMs - rateSampleAt;
    if (dt >= 250) {
      const inst = ((loaded - rateSampleLoaded) * 1000) / dt;   // pts/sec this window
      rate = rate === 0 ? inst : rate * 0.7 + inst * 0.3;       // EMA
      rateSampleAt = nowMs;
      rateSampleLoaded = loaded;
    }
    // Once fully loaded, the rate is meaningless — drop it.
    const rateStr = loaded >= target || rate < 1 ? "" : ` · ${numFmt.format(Math.round(rate))}p/s`;
    // The cap is no longer a constant — the "load all points" button lifts it — so
    // say what the current target actually is rather than hard-coding it.
    // "capped" only while a cap is actually holding points back. Compare against the
    // reachable count, not the file total: with a lines overview the stream can only
    // reach the points its windows cover, and calling that a cap would be wrong.
    const full = fullPointCount();
    const text = !full || target >= full
      ? `${numFmt.format(loaded)} of ${fmtCount(full || loaded)} points${rateStr}`
      : `${numFmt.format(loaded)} points (capped to ${fmtCount(target)})${rateStr}`;
    refreshLoadAll();
    if (text === lastText) return;   // only touch the DOM when it changes
    lastText = text;
    counterEl.textContent = text;
  }

  // --- "Load all points" button (top-right, under the opacity sliders) ---------
  // CONFIG caps the stream two ways: maxPoints (GPU buffer + fetch bound) and
  // loadTurns (stop after N rotations), so a visitor who just reads the page never
  // downloads the whole ~9.4M-point cloud. This button opts INTO the full cloud:
  // it lifts both limits, regrows the buffers, and restarts the pumps where they
  // stopped. It's one-way on purpose — points already on the GPU stay there.
  const loadAllEl = document.createElement("button");
  loadAllEl.type = "button";
  loadAllEl.className = "ms-hero__loadall";
  loadAllEl.disabled = true;
  loadAllEl.textContent = "Load all points";

  /** Compact count for labels: 9,393,443 -> "9.4m", small nets stay exact. */
  function fmtCount(n) {
    if (!(n > 0)) return "0";
    if (!Number.isFinite(n)) return "all";
    return n >= 1e6 ? `${(n / 1e6).toFixed(1).replace(/\.0$/, "")}m` : numFmt.format(n);
  }

  /** The full point count actually reachable: the file's total, bounded by the
   *  overview's coverage (Infinity until the overview lands, so this is `realTotal`
   *  in the no-overview case). */
  function fullPointCount() {
    return Math.min(realTotal || 0, coveredPoints);
  }

  /** Regrow the streaming buffers to hold `n` points, preserving what's loaded.
   *  Typed arrays can't resize, so this allocates, copies, and swaps the geometry's
   *  attributes. geom.dispose() first releases the OLD GPU buffers (the geometry
   *  itself stays usable and three.js re-uploads the new attributes on next draw);
   *  without it the previous multi-MB buffers would leak for the page's lifetime. */
  function growPointBuffers(n) {
    if (n <= positions.length / 3) return;
    const nextPositions = new Float32Array(n * 3);
    nextPositions.set(positions);
    const nextBirths = new Float32Array(n).fill(-1);
    nextBirths.set(births);
    positions = nextPositions;
    births = nextBirths;
    posAttr = new THREE.BufferAttribute(positions, 3).setUsage(THREE.DynamicDrawUsage);
    birthAttr = new THREE.BufferAttribute(births, 1).setUsage(THREE.DynamicDrawUsage);
    geom.dispose();
    geom.setAttribute("position", posAttr);
    geom.setAttribute("aBirth", birthAttr);
    geom.setDrawRange(0, Math.max(drawHigh, loaded));   // keep what's already drawn
  }

  /** Lift both caps and resume streaming to the end of the file. */
  function unlockAllPoints() {
    const full = fullPointCount();
    if (!pointHandle || !(full > target)) return false;
    // Write CONFIG too, so a later msHero.apply() (or a console reader) sees the
    // lifted limits instead of silently reinstating the cap's intent.
    CONFIG.maxPoints = full;
    CONFIG.loadTurns = 0;            // 0 = unlimited: no rotation budget
    growPointBuffers(full);
    target = full;
    // The pumps latched `done` when they hit the cap (or the turn budget); clearing
    // it restarts them at itemCursor/loaded, which is exactly where they stopped.
    done = false;
    return true;
  }

  const onLoadAll = () => { if (unlockAllPoints()) refreshLoadAll(); };
  loadAllEl.addEventListener("click", onLoadAll);
  host.appendChild(loadAllEl);

  /** Keep the button's label/enabled state in step with the stream (called from
   *  updateCounter, so it tracks the same values the readout shows). */
  function refreshLoadAll() {
    const full = fullPointCount();
    let label, disabled;
    if (!pointHandle) {
      label = "Load all points";            // total unknown until the handle opens
      disabled = true;
    } else if (target < full) {
      label = `Load all ${fmtCount(full)} points`;
      disabled = false;
    } else if (loaded >= target) {
      label = `All ${fmtCount(target)} points loaded`;
      disabled = true;
    } else {
      label = `Loading all ${fmtCount(full)} points…`;
      disabled = true;
    }
    if (loadAllEl.textContent !== label) loadAllEl.textContent = label;
    if (loadAllEl.disabled !== disabled) loadAllEl.disabled = disabled;
  }

  // --- Streaming, paced by rotation ------------------------------------------
  // Points load in file order, contiguous from 0, with the target count tied to
  // how far the globe has rotated (`swept`): one full turn loads the whole file.
  // Because the load target is a function of `swept` (= time × rotateSpeed),
  // slowing the rotation slows the load rate — one knob controls both. `swept`
  // also drives the spin (about the tilted body pole; see tick), so newly loaded
  // points keep arriving as the globe turns.
  let loaded = 0;              // points streamed in so far (contiguous from 0)
  let swept = 0;               // radians rotated since start
  let done = false;
  let inFlight = false;        // a batch read is awaiting the network

  // Kick off a batch read if one is due and none is in flight. The read is a
  // network fetch (StarDS ranged GET) and, under -sASYNCIFY, pointsReadXYZ returns
  // a Promise — so we do NOT block the render loop: the globe keeps rotating while
  // the batch is in flight, and the results are applied (and the points born, so
  // they fade in) whenever the fetch resolves.
  function pumpBatch(nowMs) {
    if (done || inFlight || !pointHandle || loaded >= target) { if (pointHandle && loaded >= target) done = true; return; }

    // How far the front (plus lead) has swept => how many points should exist.
    // Also seed a chunk immediately so the front is populated from the first
    // frames instead of waiting for the rotation to sweep them in.
    const leadAngle = CONFIG.loadLeadTurns * 2 * Math.PI;
    const wantFrac = Math.min(1, (swept + leadAngle) / (2 * Math.PI));
    const targetLoaded = Math.min(
      target,
      Math.max(CONFIG.seedPoints, Math.ceil(wantFrac * target))
    );
    if (targetLoaded <= loaded) return;

    const start = loaded;
    const want = Math.min(CONFIG.streamBatchSize, targetLoaded - loaded);
    inFlight = true;
    Promise.resolve(src.net.pointsReadXYZ(pointHandle, start, want))
      .then((batch) => {
        const got = batch.count | 0;
        if (got <= 0) { done = true; return; }

        positions.set(batch.positions.subarray(0, got * 3), start * 3);

        // Refine framing once we have real data (globe radius is ~constant).
        if (!framedFromData && batch.radius > 0) {
          radius = batch.radius;
          framedFromData = true;
          frame();
        }

        // Points are born on arrival (they fade in from here).
        const now = performance.now() / 1000;
        births.fill(now, start, start + got);

        // Upload only the touched slices (not the whole multi-MB buffer). r159+
        // uses addUpdateRange(start, count); the old `updateRange =` setter was
        // removed and assigning to it throws in a module, which would kill the loop.
        posAttr.addUpdateRange(start * 3, got * 3);
        posAttr.needsUpdate = true;
        birthAttr.addUpdateRange(start, got);
        birthAttr.needsUpdate = true;

        loaded = start + got;
        geom.setDrawRange(0, loaded);
        if (loaded >= target) done = true;
        // No overview overlay in this path — the placeholder was the standing
        // view, so fade it out now that real points are arriving.
        if (placeholder) placeholder.fadeOut(now);
      })
      .catch((err) => { done = true; console.warn("[StarDS hero] stream read failed:", err); })
      .finally(() => { inFlight = false; });
  }

  // --- Overview-driven streaming (cnet/3) ------------------------------------
  // With a "lines" or "summary" overview, the stream follows the overview's
  // per-item [rangeStart,rangeCount) windows in file order (front-first as the
  // globe turns) — the LOD-0 → LOD-1 drill-down. Rotation paces it (how many
  // items are "due" scales with `swept`); as each item's real points arrive we
  // crossfade its overview primitive (line/splat) out.
  //
  // THROUGHPUT: items are tiny (a line ≈ 40 points), so reading one item per call
  // would pay a full get_slice/block-decompress per ~40 points — ~1000x fewer
  // points/read than the fixed-batch path. To keep the old throughput we COALESCE
  // consecutive items into ONE read that spans a contiguous point window up to
  // ~streamBatchSize points, then mark every covered item arrived. Items are
  // contiguous in file order, so a run of them is a single window (any gap points
  // in between are real points too and simply stream in with the run).
  // `overview` (+ markArrived) are populated when src.overviewReady resolves;
  // until then the pump can't run because `pointHandle` (chained after the overview
  // open) is still null. markArrived crossfades an overview primitive out as its
  // real points arrive.
  let markArrived = null;
  let itemCursor = 0;    // index of the next overview item to stream
  let drawHigh = 0;      // furthest point index written (items stream out of order)
  let curBatch = CONFIG.streamBatchMin || CONFIG.streamBatchSize;  // adaptive batch
  function pumpOverview(nowMs) {
    if (done || inFlight || !overview || !pointHandle) return;  // wait for the handle
    const K = overview.count | 0;
    if (itemCursor >= K) { done = true; return; }

    // Real-point cap: once maxPoints have streamed in, STOP. `target` is already
    // min(total, maxPoints), and filament windows tile [0,total) in ascending
    // rangeStart order, so once we've filled [0,target) no later line adds real
    // points — halt now instead of spinning the skip-loop over beyond-cap items
    // every frame. Regions past the cap keep showing the line overview.
    if (loaded >= target) { done = true; return; }

    // Fetch budget: stop after `loadTurns` full rotations (0 = unlimited). A user
    // who never zooms then never pays to fetch the whole cloud.
    if (CONFIG.loadTurns > 0 && swept >= CONFIG.loadTurns * 2 * Math.PI) { done = true; return; }

    // How many items should be loaded by now (rotation-paced, + immediate seed).
    const wantFrac = Math.min(1, swept / (2 * Math.PI));
    const seedItems = Math.max(1, Math.ceil((CONFIG.seedPoints / Math.max(target, 1)) * K));
    const dueItems = Math.max(seedItems, Math.ceil(wantFrac * K));
    if (itemCursor >= dueItems) return;

    // Skip leading items whose window is out of the GPU buffer or empty.
    while (itemCursor < dueItems) {
      const rc0 = overview.rangeCount[itemCursor] >>> 0;
      const rs0 = overview.rangeStart[itemCursor] >>> 0;
      if (rc0 !== 0 && rs0 < target) break;
      itemCursor++;
    }
    if (itemCursor >= dueItems) return;

    // Coalesce a run of due items into one contiguous point window, bounded by
    // streamBatchSize. `iEnd` is the exclusive item index the run stops at.
    const runStart = overview.rangeStart[itemCursor] >>> 0;
    let iEnd = itemCursor;
    let windowEnd = runStart;
    while (iEnd < dueItems) {
      const rs = overview.rangeStart[iEnd] >>> 0;
      const rc = overview.rangeCount[iEnd] >>> 0;
      if (rs >= target) break;                       // beyond the buffer
      const end = Math.min(rs + rc, target);
      // stop if adding this item would overflow the (adaptive) batch (unless first)
      if (iEnd > itemCursor && (end - runStart) > curBatch) break;
      windowEnd = Math.max(windowEnd, end);
      iEnd++;
    }
    const start = runStart;
    const want = windowEnd - runStart;
    const firstItem = itemCursor, lastItem = iEnd;   // [firstItem, lastItem)
    if (want <= 0) { itemCursor = iEnd; return; }

    inFlight = true;
    Promise.resolve(src.net.pointsReadXYZ(pointHandle, start, want))
      .then((batch) => {
        const got = batch.count | 0;
        if (got > 0) {
          positions.set(batch.positions.subarray(0, got * 3), start * 3);
          if (!framedFromData && batch.radius > 0) {
            radius = batch.radius; framedFromData = true; frame();
          }
          const now = performance.now() / 1000;
          births.fill(now, start, start + got);
          posAttr.addUpdateRange(start * 3, got * 3); posAttr.needsUpdate = true;
          birthAttr.addUpdateRange(start, got); birthAttr.needsUpdate = true;
          const end = start + got;
          if (end > drawHigh) { drawHigh = end; geom.setDrawRange(0, drawHigh); }
          loaded += got;
          // Crossfade every item covered by this run.
          if (markArrived)
            for (let i = firstItem; i < lastItem; ++i) markArrived(i, now);
        }
        itemCursor = lastItem;
        if (itemCursor >= K) done = true;
        // Ramp the batch up so steady-state reads are fat (fewer amortized
        // round-trips); the small first reads already gave a quick first paint.
        curBatch = Math.min(CONFIG.streamBatchMax, Math.ceil(curBatch * CONFIG.streamBatchGrow));
      })
      .catch((err) => { done = true; console.warn("[StarDS hero] overview stream failed:", err); })
      .finally(() => { inFlight = false; });
  }

  // Build the overview overlay WHEN it resolves, and hand off from the placeholder.
  // This runs before src.ready sets `pointHandle` (ready is chained after the
  // overview open), so the pump can't fire until the overlay + markArrived exist.
  let realTotal = target;
  src.overviewReady
    .then((ov) => {
      const lines = ov && ov.lines, summary = ov && ov.summary;
      hasLines = CONFIG.showLines && lines && (lines.count | 0) > 0;
      hasSummary = !hasLines && CONFIG.showSplats && summary && (summary.count | 0) > 0;
      if (hasLines) {
        overview = lines;
        // Tighten target to the points the lines actually cover (≤ maxPoints); the
        // buffers were sized to maxPoints, so this only shrinks the draw target.
        const L = lines.count | 0;
        const covered = (lines.rangeStart[L - 1] >>> 0) + (lines.rangeCount[L - 1] >>> 0);
        coveredPoints = covered;   // the ceiling "load all points" can raise to
        target = Math.min(target, covered);
        lineOverlay = makeLineMesh(lines, pixelRatio);
        lineOverlay.points.renderOrder = -1;   // draw before the real points
        spin.add(lineOverlay.points);
        markArrived = (i, t) => lineOverlay.markLineArrived(i, t);
      } else if (hasSummary) {
        overview = summary;
        gmm = makeGmmCloud(summary, pixelRatio);
        if (gmm) {
          gmm.points.renderOrder = -1;
          spin.add(gmm.points);
          markArrived = (i, t) => gmm.markSplatArrived(i, t);
        }
      }
      overviewSettled = true;
      frame();   // refresh the overlay's fog uNearZ/uFarZ uniforms now it exists
      // Hand off from the placeholder ONLY when an overview overlay was actually
      // built and is now on screen. If NEITHER layer was present (empty/failed
      // net), keep the placeholder as the standing view — it otherwise fades when
      // the first real points arrive (see the pumps). This is why a bad/empty net
      // still shows the placeholder globe instead of blanking.
      const builtOverlay = (hasLines && lineOverlay) || (hasSummary && gmm);
      if (builtOverlay && placeholder) placeholder.fadeOut(performance.now() / 1000);
    })
    .catch((err) => {
      overviewSettled = true;   // no overview → let the plain pump take over
      // Do NOT fade the placeholder here: with no overview it's the fallback view
      // (it fades once real points stream in, if they do).
      console.warn("[StarDS hero] overview build failed:", err);
    });

  // Resolve the background point-stream open (chained AFTER the overview open, so
  // ASYNCIFY only ever has one suspension in flight). Until this lands the pump
  // no-ops; the overview/placeholder carry the view.
  src.ready
    .then((r) => {
      pointHandle = r.handle;
      realTotal = r.total || realTotal;
    })
    .catch((err) => { console.warn("[StarDS hero] point stream open failed:", err); });

  let raf = 0;
  let lastFrameMs = 0;
  function tick(now) {
    const dt = lastFrameMs ? Math.min(0.05, (now - lastFrameMs) / 1000) : 0;  // clamp tab-switch jumps
    lastFrameMs = now;
    // Auto-spin ACCUMULATES from the frame delta (not absolute time) so we can
    // PAUSE it while the trackpad is grabbed and resume with no jump. Held-pad
    // pauses `swept`, which also paces streaming — so point-loading pauses during
    // the drag and resumes on release (fine: you grabbed it to inspect).
    if (CONFIG.autoRotate && !padActive) swept += CONFIG.rotateSpeed * dt;
    // Spin about the body's pole (Z). `orient` leans this axis, so the globe
    // turns on a tilted, pole-up axis rather than with the poles on the side.
    spin.rotation.z = swept;

    // Hand-orbit: the trackpad drag accumulates orbitQuat (see its pointermove
    // handler), so here we just copy it onto the userOrbit group. Quaternion → no
    // gimbal lock, so the globe can be tumbled to any orientation.
    userOrbit.quaternion.copy(orbitQuat);

    // Pump the stream once an overview is resolved AND the point handle landed;
    // pumpOverview itself no-ops until `pointHandle` is set. If there's genuinely
    // no overview (both layers absent), fall back to the plain rotation pump.
    if (overview) pumpOverview(now);
    else if (pointHandle && overviewSettled) pumpBatch(now);
    material.uniforms.uTime.value = now / 1000;
    if (gmm) gmm.mat.uniforms.uTime.value = now / 1000;          // GMM fade-out
    if (lineOverlay) lineOverlay.mat.uniforms.uTime.value = now / 1000;  // line fade-out

    // Placeholder crossfade: ramp uFade 0→1 over placeholderFadeOutMs from the
    // moment the overview landed (fadeStart), then drop the mesh once fully faded.
    if (placeholder) {
      const fs = placeholder.points.userData.getFadeStart();
      if (fs >= 0) {
        const f = Math.min(1, (now / 1000 - fs) / (CONFIG.placeholderFadeOutMs / 1000));
        placeholder.mat.uniforms.uFade.value = f;
        if (f >= 1) {
          spin.remove(placeholder.points);
          placeholder.points.geometry.dispose();
          placeholder.mat.dispose();
          placeholder = null;
        }
      }
    }
    renderer.render(scene, camera);
    updateCounter(now);
    raf = requestAnimationFrame(tick);
  }
  raf = requestAnimationFrame(tick);

  // Expose the live handle so the levers can be tweaked from the console:
  //   window.msHero.CONFIG.zoom = 3; window.msHero.apply();
  window.msHero = {
    CONFIG,
    get loaded() { return loaded; },
    get total() { return realTotal; },
    // Same thing the "Load all points" button does: lift the maxPoints/loadTurns
    // caps and stream to the end. Returns false if there was nothing left to lift.
    loadAllPoints() { const ok = unlockAllPoints(); refreshLoadAll(); return ok; },
    apply() {
      material.uniforms.uSize.value = CONFIG.dotSize;
      material.uniforms.uOpacity.value = CONFIG.dotOpacity;
      material.uniforms.uColor.value.set(CONFIG.dotColor);
      material.uniforms.uFogColor.value.set(CONFIG.fogColor);
      material.uniforms.uFogStrength.value = CONFIG.fogStrength;
      material.uniforms.uFogFalloff.value = CONFIG.fogFalloff;
      material.uniforms.uFade.value = CONFIG.fadeDurationMs / 1000;
      renderer.setClearAlpha(CONFIG.clearAlpha);
      // Draw-or-not for the cloud + the overview's fade suspension (see
      // applyLayerOpacity for why zero opacity means "don't draw" here).
      applyLayerOpacity();
      if (gmm) {
        gmm.mat.uniforms.uColor.value.set(CONFIG.gmmColor);
        gmm.mat.uniforms.uOpacity.value = CONFIG.gmmOpacity;
        gmm.mat.uniforms.uFadeOut.value = CONFIG.gmmFadeOutMs / 1000;
        gmm.points.visible = CONFIG.showSplats;
      }
      if (lineOverlay) {
        lineOverlay.mat.uniforms.uColor.value.set(CONFIG.lineColor);
        lineOverlay.mat.uniforms.uOpacity.value = CONFIG.lineOpacity;
        lineOverlay.mat.uniforms.uFadeOut.value = CONFIG.lineFadeOutMs / 1000;
        lineOverlay.mat.uniforms.uFogColor.value.set(CONFIG.fogColor);
        lineOverlay.mat.uniforms.uFogStrength.value = CONFIG.fogStrength;
        lineOverlay.mat.uniforms.uFogFalloff.value = CONFIG.fogFalloff;
        lineOverlay.points.visible = CONFIG.showLines;
      }
      if (placeholder) {
        placeholder.mat.uniforms.uColor.value.set(CONFIG.placeholderColor);
        placeholder.mat.uniforms.uOpacity.value = CONFIG.placeholderOpacity;
        placeholder.mat.uniforms.uSize.value = CONFIG.placeholderDotSize;
        placeholder.mat.uniforms.uFogColor.value.set(CONFIG.fogColor);
        placeholder.mat.uniforms.uFogStrength.value = CONFIG.fogStrength;
        placeholder.mat.uniforms.uFogFalloff.value = CONFIG.fogFalloff;
        placeholder.points.visible = CONFIG.showPlaceholder;
      }
      applyRenderEdge();
      applyOrientation();
      // Keep the slider in sync if zoom was changed from the console.
      zoomInput.value = String(Math.min(ZOOM_MAX, Math.max(ZOOM_MIN, CONFIG.zoom)));
      padWrap.style.display = CONFIG.trackpad ? "" : "none";
      // Same for the opacity sliders (each is its layer's opacity outright).
      dotOpacityCtl.input.value = String(clamp01(CONFIG.dotOpacity));
      overviewOpacityCtl.input.value = String(clamp01(CONFIG.lineOpacity));
      frame();
    },
    dispose() {
      cancelAnimationFrame(raf);
      ro.disconnect();
      counterEl.remove();
      captionEl.remove();
      zoomInput.removeEventListener("input", onZoom);
      zoomWrap.remove();
      padWrap.removeEventListener("pointerdown", onPadDown);
      padWrap.removeEventListener("pointermove", onPadMove);
      padWrap.removeEventListener("pointerup", onPadUp);
      padWrap.removeEventListener("pointercancel", onPadUp);
      padWrap.remove();
      toggleEl.removeEventListener("click", onToggle);
      toggleEl.remove();
      for (const ctl of [dotOpacityCtl, overviewOpacityCtl]) {
        ctl.input.removeEventListener("input", ctl.handler);
      }
      opacityWrap.remove();
      loadAllEl.removeEventListener("click", onLoadAll);
      loadAllEl.remove();
      host.classList.remove("is-controls-open");
      geom.dispose();
      material.dispose();
      if (gmm) { gmm.points.geometry.dispose(); gmm.mat.dispose(); }
      if (lineOverlay) { lineOverlay.points.geometry.dispose(); lineOverlay.mat.dispose(); }
      if (placeholder) { placeholder.points.geometry.dispose(); placeholder.mat.dispose(); }
      renderer.dispose();
      // Close the point handle if it (or its pending open) resolved.
      if (pointHandle) { try { src.net.closePointsXYZ(pointHandle); } catch (_) {} }
      else { src.ready.then((r) => { try { src.net.closePointsXYZ(r.handle); } catch (_) {} }).catch(() => {}); }
    },
  };
}

// The canvas node we've initialized (or are mid-initializing). Material's instant
// navigation re-emits document$ — and thus re-runs boot() — on every navigation,
// INCLUDING in-page anchor clicks like the "Try the playground below" (#playground)
// button. Those don't replace the DOM, so the hero canvas is the SAME node; we must
// NOT tear the live render down and rebuild (that's what blanked it and reset the
// stream to 0). We only rebuild when the canvas is a genuinely new element (a real
// page content swap). Claimed synchronously below so a re-fire during the initial
// Three.js load can't start a second render.
let heroCanvas = null;
// The src handed to the live render, kept only so boot() can see whether its module
// load failed (see the same-canvas early return below).
let heroSrc = null;

async function boot() {
  const host = document.querySelector("[data-ms-hero]");
  const canvas = host && host.querySelector("[data-ms-hero-canvas]");

  // Navigated to a page with no hero: tear down any live render so it doesn't keep
  // rendering to a now-detached canvas (and holding the point handle open).
  if (!host || !canvas) {
    if (window.msHero) { window.msHero.dispose(); window.msHero = null; }
    heroCanvas = null;
    heroSrc = null;
    return;
  }

  // Same canvas we already own (or are initializing) → leave the running render
  // alone. This is the fix: instant-nav / same-page clicks no longer blank it.
  //
  // ONE exception: if the WASM module never loaded for that render, it's a
  // placeholder-only globe with no points coming. Before initScene stopped waiting
  // on the module, a failed load left heroCanvas null and the next document$ emit
  // retried the whole thing; keep that recovery by rebuilding in exactly that case
  // (loadStards() clears its memo on failure, so this genuinely re-fetches).
  if (heroCanvas === canvas && !(heroSrc && heroSrc.failed)) return;

  // A genuinely new canvas (real content swap), or a retry after a failed module
  // load: dispose the old render first.
  if (window.msHero) { window.msHero.dispose(); window.msHero = null; }
  heroCanvas = canvas;   // claim BEFORE the await so a re-entrant boot() no-ops
  heroSrc = null;

  try {
    // Await ONLY Three.js — never the WASM module. openStream() returns
    // synchronously with the module load still in flight, so initScene() runs as
    // soon as the (small) Three.js build lands and the placeholder globe is on
    // screen from its very first frame. The WASM-backed layers attach themselves
    // afterwards via src.overviewReady / src.ready, each of which already handles
    // its own failure — so a slow or broken module delays the points only, never
    // the render.
    await loadThree();
    if (heroCanvas !== canvas) return;   // superseded while awaiting — abandon
    heroSrc = openStream();
    initScene(canvas, host, heroSrc);
    host.setAttribute("data-ms-hero-ready", "true");
  } catch (err) {
    if (heroCanvas === canvas) { heroCanvas = null; heroSrc = null; }   // let a later emit retry
    // Non-fatal: the hero still shows its gradient + text without the render.
    console.warn("[StarDS hero] point render unavailable:", err);
  }
}

// Exported for the node-side smoke test (docs-site/tests/hero_net_smoke.mjs),
// which drives the data layer against the real net without a browser. The page
// loads this file with <script type="module" src=...>, which ignores exports.
export { makeNetApi };

if (typeof document$ !== "undefined" && document$.subscribe) {
  document$.subscribe(boot);
} else if (document.readyState !== "loading") {
  boot();
} else {
  document.addEventListener("DOMContentLoaded", boot);
}

/*
 * StarDS homepage hero — 3D point-cloud banner (Three.js).
 *
 * Two clouds share one 3D scene:
 *
 *  1. STARFIELD (background) — a regular grid of fixed-width circles at varied Z,
 *     dense toward the lower-right, accent points nearer and neutral stars
 *     deeper, all fading very slowly. This is the existing field, untouched.
 *
 *  2. FOREGROUND — a near plane that reproduces the ORIGINAL 2D banner's front
 *     layer in 3D: an fBm cloud times a dense-lower-right ramp, a random
 *     directional bias (near-solid grid at one edge rolling off), viridis
 *     colouring, and a per-point alpha where each dot is either opaque or
 *     randomly transparent — favouring opaque where the local density is low.
 *     It sits in front of the starfield (nearer Z + drawn on top).
 *
 * Because the two clouds live at different depths, scrolling the page pans the
 * camera (no zoom) and they parallax against each other naturally.
 *
 * Colours come from the hero's CSS custom properties, so switching the Material
 * palette (light/dark) recolours both fields. Falls back gracefully (flat banner
 * background) if WebGL/Three.js is unavailable, and respects reduced-motion.
 *
 * Loaded as an ES module, home page only, via overrides/home.html.
 */
import * as THREE from "three";

(function () {
  "use strict";

  // --- Shared field extents -------------------------------------------------
  var FIELD_W = 60;         // world half-extent in x (both clouds share this)
  var FIELD_H = 34;         // world half-extent in y

  // --- Starfield (background) tunables --------------------------------------
  var GRID_COLS   = 320;    // sampling-lattice resolution (x); cells are kept/dropped by the density field
  var GRID_ROWS   = 330;    // sampling-lattice resolution (y)
  var Z_MIN       = -100;   // deepest a star can sit (into the screen)
  var Z_MAX       = 100;    // nearest a star can sit
  var ACCENT_FRAC = 0.8;    // fraction of stars that take the accent ramp
  var STAR_PX     = 0.8;    // BASE marker width in CSS px (each star's size multiplies this)
  var STAR_SHAPE  = "circle"; // marker shape: "circle" or "square"
  var STAR_TW_MIN = 0.3;    // min opacity of a fading star
  var STAR_TW_SPEED = 0.05; // star fade cycles/sec — slow

  // --- Realistic star distribution ------------------------------------------
  // Real skies aren't evenly scattered: a sparse background of field stars, an
  // irregular diffuse band (Milky-Way-like), and a few tight CLUSTERS. These are
  // summed into one density field; positions are jittered off the sampling
  // lattice so no grid pattern shows through.
  var STAR_DENSITY       = 10.0;   // global density multiplier: <1 sparser sky, >1 denser (scales every layer)
  var STAR_JITTER        = 0.0;   // 0 = rigid lattice, 1 = jitter each star ~one cell (breaks the grid)
  var STAR_FIELD_DENSITY = 0.0;  // baseline keep-probability of a background field star
  var STAR_FIELD_NOISE   = 0.6;   // 0 = smooth field, 1 = strongly clumped background (fBm)
  var STAR_CLUSTERS      = 10;     // number of star clusters seeded across the field
  var STAR_CLUSTER_STRENGTH = 1.4;// peak extra density at a cluster core (>~0.8 -> near-solid knot)
  var STAR_CLUSTER_MIN_R = 0.2;  // min cluster radius (fraction of the field)
  var STAR_CLUSTER_MAX_R = 4.4;  // max cluster radius (fraction of the field)
  var STAR_CLUSTER_SPREAD = 0.6;  // how far clusters scatter from the CENTRE: 0 = all at centre,
                                  //   1 = anywhere in the field. Keeps clusters centred (no corner pile-up).
  var STAR_BAND_STRENGTH = 0.3;   // diffuse band extra density (0 = no band)
  var STAR_BAND_DIR      = [1, 0.4]; // band orientation [dx, dy] (canvas +x right, +y up)
  var STAR_BAND_WIDTH    = 0.16;  // band half-width (fraction of the field)
  var STAR_ENVELOPE      = 0.0;   // 0 = uniform sky, 1 = strong CENTRED concentration (dense at centre, thinning outward)

  // Star SIZE + brightness follow a luminosity function: faint small stars vastly
  // outnumber bright large ones. Size is a multiple of STAR_PX; brightness tracks
  // size so big stars pop while faint ones sit quietly in the background.
  var STAR_SIZE_MIN   = 0.9;   // smallest star (× STAR_PX)
  var STAR_SIZE_MAX   = 2.2;   // brightest star (× STAR_PX)
  var STAR_SIZE_POWER = 4.0;   // >1 = steep: many faint/small, few bright/large
  var STAR_DIM_ALPHA  = 0.3;   // base opacity of the faintest star (brightest = 1.0)

  // --- Foreground tunables (mirror the original 2D banner front layer) ------
  // The foreground is a SQUARE-CELL grid: FG_ROWS cells span the (constant) frustum
  // HEIGHT, and the column count is derived from the viewport aspect on every resize
  // so cell spacing stays uniform and identical in x and y. This keeps the dots'
  // size-to-gap ratio — the whole look — constant when the window is reshaped;
  // narrowing the window drops columns rather than smooshing the same grid.
  var FG_ROWS       = 80;     // grid rows across the frustum HEIGHT (the resize-invariant)
  var FG_MAX_COLS   = 400;    // safety cap on derived column count (very wide viewports)
  var FG_NOISE_BASE = 100;    // fBm frequency across the field (edge raggedness)

  // --- Foreground directional fade (how the grid thins along one direction) --
  // A SOLID grid up to FG_FADE_START, thinning to nothing by FG_FADE_END,
  // measured 0..1 along FG_FADE_DIR (0 = dense edge, 1 = thin edge). Before
  // START the grid is guaranteed hole-free.
  //   • Hard edge:        START == END (e.g. 0.5, 0.5) — solid then abruptly gone.
  //   • Subtle sparseness: spread them apart (e.g. 0.0, 1.0) — a gentle gradient.
  // FG_FADE_DIR is [dx, dy] pointing FROM the dense edge TOWARD where it thins
  // (canvas coords: +x right, +y up). Examples:
  //   [-1,0] dense-right→thin-left   [1,0] dense-left→thin-right
  //   [0,1] dense-bottom→thin-top    [0,-1] dense-top→thin-bottom   (diagonals ok)
  var FG_FADE_DIR   = [1, 1];
  var FG_FADE_START = 0.55;    // 0..1: solid grid (no holes) until here
  var FG_FADE_END   = 0.65;    // 0..1: fully thinned out by here (== START for a hard edge)
  var FG_FADE_CURVE = 1;    // how fast it thins across START→END: 1 = linear,
                              //   >1 = stays dense then drops fast near END,
                              //   <1 = drops fast right after START
  var FG_FADE_NOISE = 0.0;    // 0 = clean gradient, 1 = ragged/organic fade edge
  // Foreground colour is a single viridis GRADIENT across the whole field (every
  // square samples the shared palette by its position — not random per point).
  // FG_COLOR_DIR is the gradient direction [dx, dy] (canvas +x right, +y up);
  // FG_COLOR_SCALE stretches/compresses the ramp across that direction (1 = one
  // full palette sweep spans the field). The gradient can slowly slide as a whole
  // over time: SHIFT_SPEED cycles/sec (0 = static), all points shift together.
  var FG_COLOR_DIR     = [1, 0];
  var FG_COLOR_SCALE   = 0.4;
  var FG_COLOR_SHIFT_SPEED = 0.01;
  var FG_OPAQUE_BASE = 0.35;  // opaque probability in the densest regions
  var FG_OPAQUE_BIAS = 5.0;   // how strongly sparseness pushes toward opaque
  var FG_FADED_MIN   = 0.12;  // min alpha of a "transparent" foreground dot
  var FG_FADED_MAX   = 0.8;   // max alpha of a "transparent" foreground dot
  var FG_PX          = 10;     // marker width in CSS px at FG_REF_HEIGHT (see below)
  var FG_REF_HEIGHT  = 900;    // hero height (px) at which FG_PX is calibrated. The
                               // foreground scales with the viewport height so its
                               // density/appearance stays fixed across window sizes
                               // (shorter windows would otherwise crowd the dots).
  var FG_SHAPE       = "circle"; // marker shape: "circle" or "square"
  var FG_Z           = 75;   // near plane, in FRONT of every star (Z_MAX = 100)
  var FG_Z_JITTER    = 0.0;     // small depth spread so the plane isn't perfectly flat
  var FG_TW_MIN      = 0.6;   // foreground fades only subtly (keeps baked alpha)
  var FG_TW_SPEED    = 0.3;  // foreground fade cycles/sec — slow
  var FG_DIST        = 100;   // foreground distance IN FRONT of the camera (camera-fixed plane)
  // The foreground field fills the viewport frustum at FG_DIST, so grid coords
  // (and thus FG_FADE_START/END) map to VIEWPORT FRACTIONS and stay proportionate
  // on resize: 0 = left/bottom edge, 1 = right/top edge, updating live.
  var FG_OFFSET_X    = -0.2;   // shift the field in x, in viewport widths (+right)
  var FG_OFFSET_Y    = 0;      // shift the field in y, in viewport heights (+up)
  var FG_FILL        = 1.0;    // fraction of the viewport the field spans (1 = exactly fills)

  // --- Camera / motion ------------------------------------------------------
  var CAM_Z        = 50;   // camera distance (large -> grid stays fairly regular)
  var FOV          = 34;    // perspective FOV (deg)
  var PARALLAX_PAN = 22;    // world units the camera pans over the hero's scroll
  var DRIFT_AMP    = 2.5;   // gentle idle drift of the field
  var DRIFT_SPEED  = 0.03;  // idle drift rate

  function clamp01(v) { return v < 0 ? 0 : v > 1 ? 1 : v; }

  // Seed a handful of star clusters at random positions/radii/weights. Each is a
  // Gaussian bump in the density field, so stars pile up toward its core and thin
  // out at the edges — the way open clusters actually look. Centres scatter around
  // the FIELD CENTRE (0.5, 0.5) by ±STAR_CLUSTER_SPREAD, so the field stays
  // centred on the camera instead of piling into a corner.
  function makeClusters(rng, n) {
    var out = [];
    for (var i = 0; i < n; i++) {
      out.push({
        // centre (0..1) offset from mid by up to ±0.5*SPREAD, then clamped in-field.
        x: clamp01(0.5 + (rng() - 0.5) * STAR_CLUSTER_SPREAD),
        y: clamp01(0.5 + (rng() - 0.5) * STAR_CLUSTER_SPREAD),
        r: STAR_CLUSTER_MIN_R + rng() * (STAR_CLUSTER_MAX_R - STAR_CLUSTER_MIN_R),
        w: STAR_CLUSTER_STRENGTH * (0.5 + 0.5 * rng())        // per-cluster peak weight
      });
    }
    return out;
  }

  // Realistic sky density at (nx, ny) in 0..1 field coords. Sums three natural
  // structures into a keep-probability:
  //   1. Field stars   — a low, fBm-clumped background (never perfectly uniform).
  //   2. Diffuse band  — a soft Milky-Way-like stripe along STAR_BAND_DIR.
  //   3. Clusters      — tight Gaussian knots seeded by makeClusters().
  // An optional large-scale ENVELOPE concentrates density toward the FIELD CENTRE
  // (which maps to the camera centre), blended in by STAR_ENVELOPE (0 = even sky).
  function skyDensity(nx, ny, clusters) {
    // 1) Clumped background field.
    var clump = fbm(nx * 6 + 11.3, ny * 6 + 4.7, 7, 5);        // 0..1, smooth blobs
    var field = STAR_FIELD_DENSITY *
                (1 - STAR_FIELD_NOISE + STAR_FIELD_NOISE * clump * 2);

    // 2) Diffuse band: distance from a line through the field centre.
    var bd = Math.hypot(STAR_BAND_DIR[0], STAR_BAND_DIR[1]) || 1;
    var bx = STAR_BAND_DIR[0] / bd, by = STAR_BAND_DIR[1] / bd;
    var perp = Math.abs((nx - 0.5) * (-by) + (ny - 0.5) * bx);  // ⟂ distance to band axis
    var bandNoise = 0.6 + 0.8 * fbm(nx * 5 + 30, ny * 5 + 30, 9, 4); // ragged edges
    var band = STAR_BAND_STRENGTH *
               Math.exp(-(perp * perp) / (2 * STAR_BAND_WIDTH * STAR_BAND_WIDTH)) *
               bandNoise;

    // 3) Clusters: Gaussian bump per cluster.
    var cl = 0;
    for (var i = 0; i < clusters.length; i++) {
      var c = clusters[i];
      var dx = nx - c.x, dy = ny - c.y;
      cl += c.w * Math.exp(-(dx * dx + dy * dy) / (2 * c.r * c.r));
    }

    var density = field + band + cl;

    // Optional large-scale envelope: concentrate density toward the field CENTRE
    // (0.5, 0.5) -> the camera centre. Radial falloff, 1 at centre to ~0 at the
    // corners; blended by STAR_ENVELOPE (0 = even sky, 1 = strongly centred).
    if (STAR_ENVELOPE > 0) {
      var rx = (nx - 0.5) * 2, ry = (ny - 0.5) * 2;            // -1..1 from centre
      var env = clamp01(1 - Math.sqrt(rx * rx + ry * ry) / Math.SQRT2); // 1 centre .. 0 corner
      density *= (1 - STAR_ENVELOPE) + STAR_ENVELOPE * env;
    }
    return density * STAR_DENSITY;                             // keep-probability (may exceed 1 in cluster cores)
  }

  // --- Value-noise / fBm (for the foreground's organic cloud) ---------------
  function hash2(ix, iy, seed) {
    var h = (Math.imul(ix, 374761393) ^ Math.imul(iy, 668265263) ^
             Math.imul(seed, 362437)) | 0;
    h = Math.imul(h ^ (h >>> 13), 1274126177);
    return ((h ^ (h >>> 16)) >>> 0) / 4294967296;
  }
  function valueNoise(x, y, seed) {
    var x0 = Math.floor(x), y0 = Math.floor(y);
    var fx = x - x0, fy = y - y0;
    var sx = fx * fx * (3 - 2 * fx), sy = fy * fy * (3 - 2 * fy);
    var n00 = hash2(x0, y0, seed), n10 = hash2(x0 + 1, y0, seed);
    var n01 = hash2(x0, y0 + 1, seed), n11 = hash2(x0 + 1, y0 + 1, seed);
    var nx0 = n00 + sx * (n10 - n00), nx1 = n01 + sx * (n11 - n01);
    return nx0 + sy * (nx1 - nx0);
  }
  function fbm(x, y, seed, octaves) {
    var total = 0, amp = 1, norm = 0, freq = 1;
    for (var i = 0; i < octaves; i++) {
      total += amp * valueNoise(x * freq, y * freq, seed + i * 17);
      norm += amp; amp *= 0.5; freq *= 2;
    }
    return total / norm;
  }

  // --- Colour helpers -------------------------------------------------------
  function parseColor(str) {
    str = (str || "").trim();
    var m = str.match(/^#([0-9a-f]{6})$/i);
    if (m) {
      var n = parseInt(m[1], 16);
      return [((n >> 16) & 255) / 255, ((n >> 8) & 255) / 255, (n & 255) / 255];
    }
    m = str.match(/rgba?\(\s*([\d.]+)[,\s]+([\d.]+)[,\s]+([\d.]+)/i);
    if (m) return [(+m[1]) / 255, (+m[2]) / 255, (+m[3]) / 255];
    return [1, 1, 1];
  }
  function splitColorList(str) {
    var out = [], depth = 0, cur = "";
    for (var i = 0; i < str.length; i++) {
      var ch = str[i];
      if (ch === "(") depth++; else if (ch === ")") depth--;
      if (ch === "," && depth === 0) { out.push(cur); cur = ""; } else cur += ch;
    }
    if (cur.trim()) out.push(cur);
    return out.map(function (s) { return s.trim(); }).filter(Boolean);
  }
  function mix3(a, b, t) {
    return [a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t, a[2] + (b[2] - a[2]) * t];
  }
  function sampleRamp(ramp, u) {
    if (ramp.length === 1) return ramp[0];
    var f = clamp01(u) * (ramp.length - 1);
    var i = Math.floor(f), frac = f - i;
    if (i >= ramp.length - 1) return ramp[ramp.length - 1];
    return mix3(ramp[i], ramp[i + 1], frac);
  }
  function heroColors(hero) {
    var cs = getComputedStyle(hero);
    var square = parseColor(cs.getPropertyValue("--hero-square-color"));
    var rampRaw = splitColorList(cs.getPropertyValue("--hero-square-ramp"));
    var accent0 = cs.getPropertyValue("--hero-square-color-0").trim();
    var ramp = rampRaw.length ? rampRaw.map(parseColor)
             : (accent0 ? [parseColor(accent0)] : [square]);
    return { star: square, ramp: ramp };
  }

  // Build a 1-D gradient texture from a ramp ([r,g,b] in 0..1) for shader lookup.
  // The palette is baked MIRRORED (forward 0→1 then back 1→0) so the texture
  // tiles seamlessly: a lookup coord that wraps past the end reflects back to the
  // same colour instead of jumping to the first colour (no hard seam), whether
  // it's a static gradient (FG_COLOR_SCALE > 1) or the sliding time offset.
  function makeRampTexture(ramp) {
    var H = 256, W = H * 2, data = new Uint8Array(W * 4);
    for (var i = 0; i < W; i++) {
      // 0..H-1 = forward, H..2H-1 = reverse -> value reflects at the seam.
      var t = i < H ? i / (H - 1) : (2 * H - 1 - i) / (H - 1);
      var c = sampleRamp(ramp, t);
      data[i * 4] = c[0] * 255; data[i * 4 + 1] = c[1] * 255;
      data[i * 4 + 2] = c[2] * 255; data[i * 4 + 3] = 255;
    }
    var tex = new THREE.DataTexture(data, W, 1, THREE.RGBAFormat);
    tex.wrapS = THREE.RepeatWrapping;
    tex.minFilter = tex.magFilter = THREE.LinearFilter;
    tex.needsUpdate = true;
    return tex;
  }

  // --- Seeded RNG -----------------------------------------------------------
  function makeRng(seed) {
    var a = seed >>> 0;
    return function () {
      a |= 0; a = (a + 0x6D2B79F5) | 0;
      var t = Math.imul(a ^ (a >>> 15), 1 | a);
      t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t;
      return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
    };
  }

  var Z_MID = (Z_MIN + Z_MAX) / 2; // stars nearer than this read as accent

  // Shared shader for both clouds: fixed-width crisp markers that fade in
  // opacity. Per-point `aAlpha` (baked base opacity) multiplies the twinkle, so
  // the starfield (aAlpha=1) is unchanged while the foreground carries its own
  // per-dot opacity.
  //
  // `opts` (optional): { rampTex, shiftSpeed } enables an animated
  // colour: instead of the static per-point `aColor`, each dot samples a viridis
  // ramp texture at its per-point coord `aU`, offset by a slow per-point time
  // oscillation — so the field subtly shifts hue among the light tones.
  function makeMaterial(sizePx, dpr, twMin, twSpeed, shape, opts) {
    var circleMask = [
      "  vec2 d = gl_PointCoord - vec2(0.5);",
      "  float r = length(d);",
      "  float edge = fwidth(r);",
      "  float a = 1.0 - smoothstep(0.5 - edge, 0.5, r);"
    ].join("\n");
    var squareMask = [
      // Chebyshev distance -> square; AA'd edge so it stays crisp but not jagged.
      "  vec2 d = abs(gl_PointCoord - vec2(0.5));",
      "  float r = max(d.x, d.y);",
      "  float edge = fwidth(r);",
      "  float a = 1.0 - smoothstep(0.5 - edge, 0.5, r);"
    ].join("\n");
    var mask = shape === "square" ? squareMask : circleMask;
    var animColor = !!(opts && opts.rampTex);
    var perSize   = !!(opts && opts.perSize);   // scale each point by its aSize attribute

    var uniforms = {
      uTime:    { value: 0 },
      uSize:    { value: sizePx * dpr },
      uTwMin:   { value: twMin },
      uTwSpeed: { value: twSpeed }
    };
    if (animColor) {
      uniforms.uRamp       = { value: opts.rampTex };
      uniforms.uShiftSpeed = { value: opts.shiftSpeed };
    }

    var vs = [
      "attribute vec3 aColor;",
      "attribute float aAlpha;",
      "attribute float aPhase;",
      "attribute float aRate;",
      animColor ? "attribute float aU;" : "",
      perSize ? "attribute float aSize;" : "",
      "uniform float uTime;",
      "uniform float uSize;",
      "uniform float uTwMin;",
      "uniform float uTwSpeed;",
      animColor ? "uniform sampler2D uRamp;" : "",
      animColor ? "uniform float uShiftSpeed;" : "",
      "varying vec3 vColor;",
      "varying float vAlpha;",
      "void main() {",
      // Colour: static per-point (aColor), OR a shared spatial gradient — every
      // point samples the SAME palette by its position (aU), with one uniform
      // time offset so the whole gradient slides together (no per-point randomness).
      animColor
        ? "  float cu = aU + uTime * uShiftSpeed;\n" +
          "  vColor = texture2D(uRamp, vec2(cu, 0.5)).rgb;"
        : "  vColor = aColor;",
      "  float tw = 0.5 + 0.5 * sin(uTime * uTwSpeed * aRate * 6.2831853 + aPhase);",
      "  vAlpha = mix(uTwMin, 1.0, tw) * aAlpha;",
      "  gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0);",
      perSize ? "  gl_PointSize = uSize * aSize;" : "  gl_PointSize = uSize;",
      "}"
    ].join("\n");

    return new THREE.ShaderMaterial({
      uniforms: uniforms,
      transparent: true,
      depthWrite: false,
      depthTest: false,
      blending: THREE.NormalBlending,
      vertexShader: vs,
      fragmentShader: [
        "varying vec3 vColor;",
        "varying float vAlpha;",
        "void main() {",
        mask,
        "  if (a < 0.01) discard;",
        "  gl_FragColor = vec4(vColor, a * vAlpha);",
        "}"
      ].join("\n"),
      extensions: { derivatives: true }
    });
  }

  // Turn parallel per-point arrays into a Three.Points object with the shared
  // attribute layout. `arr` = { pos, col, alpha, phase, rate, [u] } typed arrays,
  // already trimmed to `count`. `arr.u` (ramp coord) is added when present, for
  // the animated-colour foreground.
  function makePoints(arr, count, material, renderOrder) {
    var geom = new THREE.BufferGeometry();
    geom.setAttribute("position", new THREE.BufferAttribute(arr.pos, 3));
    geom.setAttribute("aColor", new THREE.BufferAttribute(arr.col, 3));
    geom.setAttribute("aAlpha", new THREE.BufferAttribute(arr.alpha, 1));
    geom.setAttribute("aPhase", new THREE.BufferAttribute(arr.phase, 1));
    geom.setAttribute("aRate", new THREE.BufferAttribute(arr.rate, 1));
    if (arr.u) geom.setAttribute("aU", new THREE.BufferAttribute(arr.u, 1));
    if (arr.size) geom.setAttribute("aSize", new THREE.BufferAttribute(arr.size, 1));
    var pts = new THREE.Points(geom, material);
    pts.renderOrder = renderOrder;
    return { points: pts, geom: geom, count: count };
  }

  function init() {
    var hero = document.getElementById("star-hero");
    var canvas = document.getElementById("star-hero-canvas");
    if (!hero || !canvas) return;

    var reduce = window.matchMedia &&
                 window.matchMedia("(prefers-reduced-motion: reduce)").matches;

    var renderer;
    try {
      renderer = new THREE.WebGLRenderer({ canvas: canvas, antialias: true, alpha: true });
    } catch (e) {
      return; // no WebGL — leave the flat banner background as-is
    }
    var dpr = Math.min(window.devicePixelRatio || 1, 2);
    renderer.setPixelRatio(dpr);

    var scene = new THREE.Scene();
    var camera = new THREE.PerspectiveCamera(FOV, 1, 0.1, 800);
    camera.position.set(0, 0, CAM_Z);
    scene.add(camera);   // so camera-parented children (the foreground) render

    var colors = heroColors(hero);
    var recolorFns = [];   // per-cloud recolour closures (called on theme toggle)
    var materials = [];    // all point materials, for the shared uTime update
    var fgSizeScaler = null; // foreground material whose uSize scales with height
    var layoutFns = [];    // per-cloud layout closures (called on resize)

    // ======================================================================
    // 1) STARFIELD (background) — a realistic sky: clumped field stars, a diffuse
    //    Milky-Way-like band, and a few tight clusters, with sizes/brightness
    //    following a luminosity function (many faint, few bright).
    // ======================================================================
    (function buildStarfield() {
      // Per-load random seed so the sky (field stars, band, and cluster
      // placement) is subtly different every visit. crypto where available,
      // Math.random() otherwise.
      var starSeed;
      if (window.crypto && window.crypto.getRandomValues) {
        starSeed = window.crypto.getRandomValues(new Uint32Array(1))[0];
      } else {
        starSeed = (Math.random() * 0x100000000) >>> 0;
      }
      var rng = makeRng(starSeed);
      var clusters = makeClusters(rng, STAR_CLUSTERS);
      var maxN = GRID_COLS * GRID_ROWS;
      var pos = new Float32Array(maxN * 3);
      var col = new Float32Array(maxN * 3);
      var alpha = new Float32Array(maxN);
      var phase = new Float32Array(maxN);
      var rate = new Float32Array(maxN);
      var size = new Float32Array(maxN);    // per-star size multiple of STAR_PX
      var isAccent = new Uint8Array(maxN);

      // Jitter amplitude: one sampling-cell in each axis (×STAR_JITTER), so stars
      // scatter off the lattice and no grid shows through.
      var jx = STAR_JITTER * (FIELD_W * 2 / Math.max(1, GRID_COLS - 1));
      var jy = STAR_JITTER * (FIELD_H * 2 / Math.max(1, GRID_ROWS - 1));

      var made = 0;
      for (var gy = 0; gy < GRID_ROWS; gy++) {
        var ny = GRID_ROWS > 1 ? gy / (GRID_ROWS - 1) : 0;
        for (var gx = 0; gx < GRID_COLS; gx++) {
          var nx = GRID_COLS > 1 ? gx / (GRID_COLS - 1) : 0;
          // Keep/drop this cell by the realistic sky-density field (field stars +
          // band + clusters). Cluster cores exceed 1 -> effectively always kept.
          if (rng() > skyDensity(nx, ny, clusters)) continue;

          var i3 = made * 3;
          // Jitter off the lattice so the underlying grid is invisible.
          pos[i3]     = (nx - 0.5) * 2 * FIELD_W + (rng() - 0.5) * jx;
          pos[i3 + 1] = (ny - 0.5) * 2 * FIELD_H + (rng() - 0.5) * jy;

          // Luminosity function: u^power skews toward small -> many faint stars,
          // few bright ones. Size drives both apparent radius and base brightness.
          var lum = Math.pow(rng(), STAR_SIZE_POWER);           // 0 (faint) .. 1 (bright), skewed
          var sizeMul = STAR_SIZE_MIN + lum * (STAR_SIZE_MAX - STAR_SIZE_MIN);
          size[made] = sizeMul;

          // Brighter (larger) stars sit nearer; accent stars are the nearer ones.
          var accent = rng() < ACCENT_FRAC;
          var z = accent ? Z_MID + rng() * (Z_MAX - Z_MID)
                         : Z_MIN + rng() * (Z_MID - Z_MIN);
          pos[i3 + 2] = z;
          isAccent[made] = accent ? 1 : 0;

          var c;
          if (accent) {
            c = sampleRamp(colors.ramp, clamp01((z - Z_MIN) / (Z_MAX - Z_MIN)));
          } else {
            var j = 0.7 + rng() * 0.6;
            c = [colors.star[0] * j, colors.star[1] * j, colors.star[2] * j];
          }
          col[i3] = c[0]; col[i3 + 1] = c[1]; col[i3 + 2] = c[2];
          // Base opacity tracks luminosity: faint stars are dim, bright ones solid.
          alpha[made] = STAR_DIM_ALPHA + lum * (1 - STAR_DIM_ALPHA);
          phase[made] = rng() * Math.PI * 2;
          rate[made] = 0.6 + rng() * 0.9;
          made++;
        }
      }
      var mat = makeMaterial(STAR_PX, dpr, STAR_TW_MIN, reduce ? 0 : STAR_TW_SPEED,
                             STAR_SHAPE, { perSize: true });
      var cloud = makePoints({
        pos: pos.subarray(0, made * 3), col: col.subarray(0, made * 3),
        alpha: alpha.subarray(0, made), phase: phase.subarray(0, made),
        rate: rate.subarray(0, made), size: size.subarray(0, made)
      }, made, mat, 0);
      scene.add(cloud.points);
      materials.push(mat);

      recolorFns.push(function (cols) {
        var ca = cloud.geom.getAttribute("aColor");
        var p = cloud.geom.getAttribute("position");
        for (var i = 0; i < cloud.count; i++) {
          var z = p.getZ(i), c;
          if (isAccent[i]) {
            c = sampleRamp(cols.ramp, clamp01((z - Z_MIN) / (Z_MAX - Z_MIN)));
          } else {
            c = cols.star;
          }
          ca.setXYZ(i, c[0], c[1], c[2]);
        }
        ca.needsUpdate = true;
      });
    })();

    // ======================================================================
    // 2) FOREGROUND — the original 2D banner front layer, rebuilt in 3D on a
    //    near plane. fBm cloud * dense-lower-right ramp, directional bias,
    //    viridis colouring, per-point opaque/transparent alpha.
    // ======================================================================
    (function buildForeground() {
      // Per-load random seed so the foreground distribution is subtly different
      // every visit. Chosen ONCE here and reused by every regenerate() call, so a
      // resize reshapes the grid resolution without reshuffling the pattern. Uses
      // crypto for good entropy where available, falling back to Math.random().
      var FG_SEED;
      if (window.crypto && window.crypto.getRandomValues) {
        FG_SEED = window.crypto.getRandomValues(new Uint32Array(1))[0];
      } else {
        FG_SEED = (Math.random() * 0x100000000) >>> 0;
      }

      // Directional density fade driven by FG_FADE_DIR: solid grid up to
      // FG_FADE_START, thinning to zero by FG_FADE_END. Grid/screen space:
      // nx 0=left..1=right, ny 0=BOTTOM..1=top; FG_FADE_DIR is canvas +y-up, so
      // it maps directly. `dir` points from the dense edge toward the thin edge.
      // These constants are aspect-independent (nx/ny always span [0,1]), so they
      // hold across regenerations.
      var dir = FG_FADE_DIR;
      var dlen = Math.hypot(dir[0], dir[1]) || 1;
      var ddx = dir[0] / dlen, ddy = dir[1] / dlen;      // unit fade direction
      var pMin = Math.min(0, ddx) + Math.min(0, ddy);
      var pMax = Math.max(0, ddx) + Math.max(0, ddy);
      var pSpan = (pMax - pMin) || 1;
      var fadeLen = FG_FADE_END - FG_FADE_START;         // 0 => hard edge

      // Colour-gradient direction (independent of the fade direction).
      var clen = Math.hypot(FG_COLOR_DIR[0], FG_COLOR_DIR[1]) || 1;
      var cdx = FG_COLOR_DIR[0] / clen, cdy = FG_COLOR_DIR[1] / clen;
      var cMin = Math.min(0, cdx) + Math.min(0, cdy);
      var cMax = Math.max(0, cdx) + Math.max(0, cdy);
      var cSpan = (cMax - cMin) || 1;

      var rampTex = makeRampTexture(colors.ramp);
      var mat = makeMaterial(FG_PX, dpr, FG_TW_MIN, reduce ? 0 : FG_TW_SPEED, FG_SHAPE,
        { rampTex: rampTex, shiftSpeed: reduce ? 0 : FG_COLOR_SHIFT_SPEED });
      // The foreground's on-screen spacing scales with viewport height (fixed
      // camera distance + perspective), so scale its marker size the same way to
      // keep the dot-to-gap ratio — hence the whole look — constant on any size.
      fgSizeScaler = mat;

      // One persistent Points object; its geometry is regenerated when the column
      // count changes (see rebuild()). renderOrder 1 -> drawn on top of the stars.
      var points = new THREE.Points(new THREE.BufferGeometry(), mat);
      points.renderOrder = 1;
      points.frustumCulled = false;   // camera-parented; skip the (now-stale) cull test
      camera.add(points);             // parent to the camera -> no parallax
      materials.push(mat);

      var nxArr = null, nyArr = null, count = 0, curCols = -1;

      // Regenerate the grid for `cols` columns × FG_ROWS rows. The density fade,
      // per-point alpha and colour are all functions of nx/ny in [0,1], so the
      // PATTERN is identical regardless of column count — only the sampling
      // resolution changes. Deterministic: the RNG is reseeded each call.
      function regenerate(cols) {
        var rng = makeRng(FG_SEED);
        var maxN = cols * FG_ROWS;
        var pos = new Float32Array(maxN * 3);
        var col = new Float32Array(maxN * 3);
        var alpha = new Float32Array(maxN);
        var phase = new Float32Array(maxN);
        var rate = new Float32Array(maxN);
        var uArr = new Float32Array(maxN);
        var nxs = new Float32Array(maxN);
        var nys = new Float32Array(maxN);

        var made = 0;
        for (var gy = 0; gy < FG_ROWS; gy++) {
          var ny = FG_ROWS > 1 ? gy / (FG_ROWS - 1) : 0;
          for (var gx = 0; gx < cols; gx++) {
            var nx = cols > 1 ? gx / (cols - 1) : 0;

            // edge01: 0 at the dense edge, 1 at the thin edge, along FG_FADE_DIR.
            var edge01 = ((nx * ddx + ny * ddy) - pMin) / pSpan;

            var keepProb;
            if (edge01 <= FG_FADE_START) {
              keepProb = 1;
            } else if (fadeLen <= 0) {
              keepProb = 0;                                 // hard edge
            } else {
              var s = clamp01((edge01 - FG_FADE_START) / fadeLen);
              keepProb = Math.pow(1 - s, FG_FADE_CURVE);
              if (FG_FADE_NOISE > 0) {
                var nz = fbm(nx * FG_NOISE_BASE + 0.5, ny * FG_NOISE_BASE + 0.5, 1, 6);
                keepProb = clamp01(keepProb + (nz - 0.5) * FG_FADE_NOISE);
              }
            }
            if (keepProb < 1 && rng() >= keepProb) continue;
            var weight = keepProb;

            var i3 = made * 3;
            nxs[made] = nx;
            nys[made] = ny;
            pos[i3 + 2] = -FG_DIST + (rng() - 0.5) * 2 * FG_Z_JITTER;

            var opaqueProb = clamp01(FG_OPAQUE_BASE + (1 - FG_OPAQUE_BASE) *
                                     Math.pow(1 - clamp01(weight), FG_OPAQUE_BIAS));
            alpha[made] = (rng() < opaqueProb)
              ? 1 : FG_FADED_MIN + rng() * (FG_FADED_MAX - FG_FADED_MIN);

            var cproj = ((nx * cdx + ny * cdy) - cMin) / cSpan;
            var u = cproj * FG_COLOR_SCALE;
            uArr[made] = u;
            var c = sampleRamp(colors.ramp, clamp01(u % 1));
            col[i3] = c[0]; col[i3 + 1] = c[1]; col[i3 + 2] = c[2];

            phase[made] = rng() * Math.PI * 2;
            rate[made] = 0.6 + rng() * 0.9;
            made++;
          }
        }

        // Swap in a fresh geometry sized to the kept points; dispose the old one.
        var geom = new THREE.BufferGeometry();
        geom.setAttribute("position", new THREE.BufferAttribute(pos.subarray(0, made * 3), 3));
        geom.setAttribute("aColor",   new THREE.BufferAttribute(col.subarray(0, made * 3), 3));
        geom.setAttribute("aAlpha",   new THREE.BufferAttribute(alpha.subarray(0, made), 1));
        geom.setAttribute("aPhase",   new THREE.BufferAttribute(phase.subarray(0, made), 1));
        geom.setAttribute("aRate",    new THREE.BufferAttribute(rate.subarray(0, made), 1));
        geom.setAttribute("aU",       new THREE.BufferAttribute(uArr.subarray(0, made), 1));
        var old = points.geometry;
        points.geometry = geom;
        if (old) old.dispose();

        nxArr = nxs; nyArr = nys; count = made; curCols = cols;
      }

      // Position the current points to fill the viewport frustum at FG_DIST.
      function layout(w, h) {
        var halfH = FG_DIST * Math.tan((FOV * Math.PI / 180) / 2); // constant
        var halfW = halfH * (w / h);
        var spanX = 2 * halfW * FG_FILL;   // viewport width in world units
        var spanY = 2 * halfH * FG_FILL;   // viewport height in world units
        var offX = FG_OFFSET_X * spanX;
        var offY = FG_OFFSET_Y * spanY;
        var p = points.geometry.getAttribute("position");
        for (var i = 0; i < count; i++) {
          p.setX(i, (nxArr[i] - 0.5) * spanX + offX);
          p.setY(i, (nyArr[i] - 0.5) * spanY + offY);
        }
        p.needsUpdate = true;
      }

      // On resize: pick the column count that makes cells SQUARE for the current
      // aspect (FG_ROWS cells span the constant frustum height, so matching x/y
      // spacing needs cols-1 = (FG_ROWS-1) * aspect). Regenerate only when that
      // count changes; then reposition. Narrowing the window therefore DROPS
      // columns instead of squeezing the same grid — spacing (and the dot-to-gap
      // ratio) stays constant, so proportions are preserved.
      layoutFns.push(function (w, h) {
        var aspect = w / h;
        var cols = Math.round((FG_ROWS - 1) * aspect) + 1;
        cols = Math.max(2, Math.min(FG_MAX_COLS, cols));
        if (cols !== curCols) regenerate(cols);
        layout(w, h);
      });

      // On theme toggle, rebuild the ramp texture the shader samples from.
      recolorFns.push(function (cols) {
        var oldTex = mat.uniforms.uRamp.value;
        mat.uniforms.uRamp.value = makeRampTexture(cols.ramp);
        if (oldTex) oldTex.dispose();
      });
    })();

    // --- Sizing --------------------------------------------------------------
    function resize() {
      var w = hero.clientWidth, h = hero.clientHeight;
      if (w <= 0 || h <= 0) return;
      renderer.setSize(w, h, false);
      camera.aspect = w / h;
      camera.updateProjectionMatrix();
      // Scale the foreground markers with viewport height so their size relative
      // to the (height-driven) spacing stays constant — keeps density/appearance
      // fixed from wide desktops down to short mobile viewports.
      if (fgSizeScaler) {
        fgSizeScaler.uniforms.uSize.value = FG_PX * dpr * (h / FG_REF_HEIGHT);
      }
      // Re-lay the foreground to fill the new viewport (keeps it proportionate).
      for (var i = 0; i < layoutFns.length; i++) layoutFns[i](w, h);
    }
    resize();
    var rt = null;
    window.addEventListener("resize", function () {
      if (rt) cancelAnimationFrame(rt);
      rt = requestAnimationFrame(resize);
    }, { passive: true });

    // --- Recolour both clouds on light/dark toggle --------------------------
    var scheme = document.body.getAttribute("data-md-color-scheme");
    new MutationObserver(function () {
      var s = document.body.getAttribute("data-md-color-scheme");
      if (s === scheme) return;
      scheme = s;
      colors = heroColors(hero);
      for (var i = 0; i < recolorFns.length; i++) recolorFns[i](colors);
    }).observe(document.body, { attributes: true, attributeFilter: ["data-md-color-scheme"] });

    // --- Scroll parallax: PAN the camera (no zoom) as the hero scrolls -------
    var scrollY = 0;
    function onScroll() { scrollY = window.scrollY || window.pageYOffset || 0; }
    window.addEventListener("scroll", onScroll, { passive: true });
    onScroll();

    // --- Render loop ---------------------------------------------------------
    var start = null;
    function frame(ts) {
      if (start === null) start = ts;
      var t = (ts - start) / 1000;
      for (var i = 0; i < materials.length; i++) {
        materials[i].uniforms.uTime.value = t;
      }

      var driftX = reduce ? 0 : Math.sin(t * DRIFT_SPEED * 6.2831853) * DRIFT_AMP;
      var driftY = reduce ? 0 : Math.cos(t * DRIFT_SPEED * 5.0) * (DRIFT_AMP * 0.6);

      // Parallax: pan the camera vertically with scroll. Camera Z fixed (no zoom)
      // and NO rotation (lookAt would tilt the camera-parented foreground) — the
      // camera only TRANSLATES, so perspective still shifts the far starfield
      // while the camera-fixed foreground stays locked to the view.
      var heroH = hero.clientHeight || 1;
      var p = clamp01(scrollY / heroH);
      camera.position.x = driftX;
      camera.position.y = driftY - p * PARALLAX_PAN;
      camera.position.z = CAM_Z;
      camera.rotation.set(0, 0, 0);   // keep axis-aligned; foreground stays flat-on

      renderer.render(scene, camera);
      requestAnimationFrame(frame);
    }
    requestAnimationFrame(frame);
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", init);
  } else {
    init();
  }
})();

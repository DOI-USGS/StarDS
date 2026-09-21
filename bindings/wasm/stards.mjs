// Convenience loader for the StarDS WebAssembly (embind) module that makes thrown
// C++ exceptions legible from JavaScript.
//
// Why this exists: built with -fexceptions, embind surfaces a thrown C++ exception
// as a raw pointer (a JS number), not an Error — so a bare `catch (e)` sees e.g.
// `146664`. -sEXPORT_EXCEPTION_HANDLING_HELPERS exposes Module.getExceptionMessage(e),
// which decodes that pointer to ['std::runtime_error', '<what()>']. This module
// wraps the Dataset class so every method AND the constructor rethrow a real JS
// Error carrying that message — for both synchronous calls and the ASYNCIFY ones
// that return Promises (open/get/put over the network).
//
// Usage:
//   import { loadStarDS } from '../stards.mjs';
//   const { Dataset, Module, decodeException } = await loadStarDS();
//   const ds = await Dataset('https://.../foo.stards');   // or new Dataset(...)
//   try { ds.get('missing'); } catch (e) { console.error(e.message); }
import initStarDS from './stards_wasm.mjs';

// Turn whatever embind threw into a real Error. Numbers are exception pointers to
// decode; arrays are already-decoded ['type','what()'] messages; anything else
// (a genuine JS Error) passes through untouched.
export function decodeException(Module, e) {
  if (typeof e === 'number' && Module.getExceptionMessage) {
    try { return new Error(Module.getExceptionMessage(e).join(': ')); }
    catch { return new Error('wasm exception #' + e); }
  }
  if (Array.isArray(e)) return new Error(e.join(': '));
  return e;
}

// TypedArray constructor name -> StarDS dtype. A TypedArray is self-describing (its
// kind IS its element type), exactly like a numpy array's .dtype in the Python
// bindings — so we can infer the dtype from it with no ambiguity.
const TYPED_ARRAY_DTYPE = {
  Int8Array: 'int8', Int16Array: 'int16', Int32Array: 'int32',
  Uint8Array: 'uint8', Uint8ClampedArray: 'uint8', Uint16Array: 'uint16', Uint32Array: 'uint32',
  Float32Array: 'float32', Float64Array: 'float64',
  BigInt64Array: 'int64', BigUint64Array: 'uint64',
};

// Infer the on-disk dtype from a JS value when the caller didn't pass one.
// TypedArrays map exactly; a plain Array (like a Python list) has no element type,
// so numeric arrays default to 'float64' (exact for integers up to 2^53, no int
// overflow, and it keeps the fast bulk-copy path) — pass an explicit dtype for a
// specific integer type, just as numpy needs np.array(list, dtype=...).
function inferDtype(value) {
  const ctorName = value?.constructor?.name;
  if (ctorName && TYPED_ARRAY_DTYPE[ctorName]) return TYPED_ARRAY_DTYPE[ctorName];
  if (typeof value === 'string') return 'string';
  if (typeof value === 'bigint') return 'int64';
  if (typeof value === 'number') return 'float64';
  if (Array.isArray(value)) {
    if (value.length === 0) return 'float64';
    const first = value[0];
    if (typeof first === 'string') return 'string';
    if (typeof first === 'bigint') return 'int64';
    return 'float64'; // numbers
  }
  throw new Error(`cannot infer dtype from value of type ${ctorName ?? typeof value}; pass an explicit dtype`);
}

// A nested array is an Array whose first element is itself array-like (Array or
// TypedArray) — e.g. [[1,2,3],[4,5,6]]. Its shape and flat data are derived from
// the nesting; a caller-supplied shape is then ignored.
function isNested(v) {
  return Array.isArray(v) && v.length > 0 && (Array.isArray(v[0]) || ArrayBuffer.isView(v[0]));
}

// Descend the first element per axis to get {shape, container, scalar}: `container`
// is the innermost array-like (used to infer dtype if it's a TypedArray), `scalar`
// the first leaf value. O(ndim), not O(n).
function nestedInfo(v) {
  const shape = [];
  let cur = v;
  let container = v;
  while (Array.isArray(cur) || ArrayBuffer.isView(cur)) {
    shape.push(cur.length);
    container = cur;
    cur = cur[0];
  }
  return { shape, container, scalar: cur };
}

// Flatten a nested array depth-first into `out` (row-major). O(n).
function flattenInto(v, out) {
  if (Array.isArray(v) || ArrayBuffer.isView(v)) {
    for (let i = 0; i < v.length; i++) flattenInto(v[i], out);
  } else {
    out.push(v);
  }
  return out;
}

// Resolve (value, shape, dtype) for put()/NDArray(): flatten a nested array (shape
// derived from nesting, dtype from its innermost container/leaf); otherwise pass the
// value through with an omitted shape -> null (1-D) and an omitted dtype -> inferred.
function normalizeData(value, shape, dtype) {
  if (isNested(value)) {
    const { shape: derived, container, scalar } = nestedInfo(value);
    const dt = dtype
      ?? (ArrayBuffer.isView(container)
            ? (TYPED_ARRAY_DTYPE[container.constructor.name] ?? 'float64')
            : inferDtype(scalar));
    return [flattenInto(value, []), derived, dt];
  }
  return [value, shape ?? null, dtype ?? inferDtype(value)];
}

// True if `x` is an NDArray handle (works through the wrapping Proxy — instanceof
// consults the prototype, which the Proxy doesn't trap).
function isNDArray(Module, x) {
  return x instanceof Module.NDArray;
}

// Polymorphic put(key, value, shape?, dtype?): an NDArray goes straight to the C++
// putArray; anything else is normalized (dtype inferred, shape defaulted, nested
// flattened) and sent to the flat put. `target` is the raw (un-proxied) instance.
function invokePut(Module, target, key, value, shape, dtype) {
  if (isNDArray(Module, value)) return target.putArray(key, value);
  const [v, s, dt] = normalizeData(value, shape, dtype);
  return target.put(key, v, s, dt);
}

// Polymorphic metaPut(key, value, dtype?). The metadata put path has no shape arg,
// so an NDArray (or a nested array, via a temporary NDArray) routes to metaPutArray
// for N-D; a scalar/1-D value uses the flat metaPut.
function invokeMetaPut(Module, target, key, value, dtype) {
  if (isNDArray(Module, value)) return target.metaPutArray(key, value);
  if (isNested(value)) {
    const [v, s, dt] = normalizeData(value, null, dtype);
    const nd = new Module.NDArray(v, s, dt);
    try { return target.metaPutArray(key, nd); } finally { nd.delete(); }
  }
  return target.metaPut(key, value, dtype ?? inferDtype(value));
}

// An embind handle is a JS object exposing a .delete() (e.g. a returned Layer).
// Wrap those too so their methods get the same error decoding; leave plain values
// (typed arrays, {data,shape}, fileHeader objects, primitives) untouched.
function maybeWrap(Module, x) {
  if (x && typeof x === 'object' && typeof x.delete === 'function') return wrapInstance(Module, x);
  return x;
}

// Proxy an embind instance so each method call decodes a sync throw or an async
// rejection, and any returned embind handle (e.g. from getLayer) is itself wrapped.
// `this` stays bound to the real object (methods applied on target), so .delete()
// and the rest behave normally.
function wrapInstance(Module, obj) {
  return new Proxy(obj, {
    get(target, prop, receiver) {
      // The NDArray-only entry points are folded into put()/metaPut(); hide them.
      if (prop === 'putArray' || prop === 'metaPutArray') return undefined;
      const v = Reflect.get(target, prop, receiver);
      if (typeof v !== 'function') return v;
      return (...args) => {
        try {
          let r;
          // put()/metaPut() are polymorphic: they accept raw values or an NDArray.
          if (prop === 'put') r = invokePut(Module, target, ...args);
          else if (prop === 'metaPut') r = invokeMetaPut(Module, target, ...args);
          else r = v.apply(target, args);
          if (r && typeof r.then === 'function') {
            return r.then((x) => maybeWrap(Module, x), (e) => { throw decodeException(Module, e); });
          }
          return maybeWrap(Module, r);
        } catch (e) {
          throw decodeException(Module, e);
        }
      };
    },
  });
}

export async function loadStarDS() {
  const Module = await initStarDS();
  const RawDataset = Module.Dataset;

  // A Dataset factory whose instances rethrow decoded Errors. The embind
  // constructor may itself return a Promise (ASYNCIFY open), so handle both a
  // resolved instance and a synchronous one. Callable with or without `new`.
  function Dataset(...args) {
    try {
      const inst = new RawDataset(...args);
      if (inst && typeof inst.then === 'function') {
        return inst.then(
          (i) => wrapInstance(Module, i),
          (e) => { throw decodeException(Module, e); },
        );
      }
      return wrapInstance(Module, inst);
    } catch (e) {
      throw decodeException(Module, e);
    }
  }

  // create(path, config) -> a wrapped Dataset (same error-decoding as above).
  // config is a Module.StarConfig instance; compression uses Module.Compression.
  function create(path, config) {
    try {
      const inst = Module.create(path, config);
      if (inst && typeof inst.then === 'function') {
        return inst.then(
          (i) => wrapInstance(Module, i),
          (e) => { throw decodeException(Module, e); },
        );
      }
      return wrapInstance(Module, inst);
    } catch (e) {
      throw decodeException(Module, e);
    }
  }

  // openBytes(uint8Array) -> a wrapped read-only Dataset (same error decoding).
  function openBytes(bytes) {
    try {
      const inst = Module.openBytes(bytes);
      if (inst && typeof inst.then === 'function') {
        return inst.then(
          (i) => wrapInstance(Module, i),
          (e) => { throw decodeException(Module, e); },
        );
      }
      return wrapInstance(Module, inst);
    } catch (e) {
      throw decodeException(Module, e);
    }
  }

  // Wrap a module-level free function so its throws decode too (only dtypeSize can
  // throw, but wrap all for uniformity).
  const wrapFn = (fn) => (...args) => {
    try {
      const r = fn(...args);
      if (r && typeof r.then === 'function') return r.then((x) => x, (e) => { throw decodeException(Module, e); });
      return r;
    } catch (e) {
      throw decodeException(Module, e);
    }
  };

  // NDArray(value, shape?, dtype?) -> a wrapped NDArray handle (decoded errors), with
  // wrapped .zeros/.ones/.full factories attached. `shape` defaults to 1-D of the
  // data length and `dtype` is inferred from `value` when omitted (see inferDtype).
  // A nested array (e.g. [[1,2,3],[4,5,6]]) is flattened with its shape derived from
  // the nesting (any passed shape is ignored). Instances from ds.getArray() are
  // already wrapped by wrapInstance's return-handling.
  const NDArray = (value, shape, dtype) =>
    wrapInstance(Module, (() => {
      const [v, s, dt] = normalizeData(value, shape, dtype);
      try { return new Module.NDArray(v, s, dt); }
      catch (e) { throw decodeException(Module, e); }
    })());
  // Factories have no data to infer from, so dtype defaults to 'float64'.
  NDArray.zeros = (shape, dtype = 'float64') => wrapInstance(Module, wrapFn(() => Module.NDArray.zeros(shape, dtype))());
  NDArray.ones = (shape, dtype = 'float64') => wrapInstance(Module, wrapFn(() => Module.NDArray.ones(shape, dtype))());
  NDArray.full = (shape, value, dtype = 'float64') => wrapInstance(Module, wrapFn(() => Module.NDArray.full(shape, value, dtype))());

  return {
    Module,
    Dataset,
    create,
    openBytes,
    NDArray,
    libraryVersion: wrapFn(Module.libraryVersion),
    networkRequestCount: wrapFn(Module.networkRequestCount),
    resetNetworkRequestCount: wrapFn(Module.resetNetworkRequestCount),
    dtypeSize: wrapFn(Module.dtypeSize),
    decodeException: (e) => decodeException(Module, e),
  };
}

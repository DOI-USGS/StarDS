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
      const v = Reflect.get(target, prop, receiver);
      if (typeof v !== 'function') return v;
      return (...args) => {
        try {
          const r = v.apply(target, args);
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

  // NDArray(value, shape, dtype) -> a wrapped NDArray handle (decoded errors), with
  // wrapped .zeros/.ones/.full factories attached. Instances from ds.getArray() are
  // already wrapped by wrapInstance's return-handling.
  const NDArray = (value, shape, dtype) =>
    wrapInstance(Module, (() => {
      try { return new Module.NDArray(value, shape, dtype); }
      catch (e) { throw decodeException(Module, e); }
    })());
  NDArray.zeros = (shape, dtype) => wrapInstance(Module, wrapFn(() => Module.NDArray.zeros(shape, dtype))());
  NDArray.ones = (shape, dtype) => wrapInstance(Module, wrapFn(() => Module.NDArray.ones(shape, dtype))());
  NDArray.full = (shape, value, dtype) => wrapInstance(Module, wrapFn(() => Module.NDArray.full(shape, value, dtype))());

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

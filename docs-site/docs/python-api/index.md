# Python API

The `pystards` package provides a Pythonic, NumPy-friendly interface to StarDS.
This page is generated from the package docstrings.

!!! note
    The API reference below is rendered by
    [mkdocstrings](https://mkdocstrings.github.io/). Because `pystards` loads a
    compiled SWIG extension at import time, the built `_pystards` module must be
    importable when the docs are built (see
    [Building the docs](#building-this-page)).

## Quick reference

| Class / function | Purpose |
|------------------|---------|
| `StarDataset` | Create, open, read, and write `.stards` files |
| `NDArray` | NumPy-compatible array wrapper |
| `MetadataValue` | Type-erased metadata container |
| `zeros`, `ones`, `arange`, `full` | Array creation helpers |
| `DataType`, `CompressionAlgorithm`, `FileMode` | Enumerations |
| `set_log_level`, `get_log_level`, `LogLevel` | Runtime logging control |

## StarDataset

::: pystards.dataset.StarDataset
    options:
      heading_level: 3

## NDArray

::: pystards.ndarray.NDArray
    options:
      heading_level: 3

### Array creation helpers

::: pystards.ndarray.zeros
    options:
      heading_level: 4
::: pystards.ndarray.ones
    options:
      heading_level: 4
::: pystards.ndarray.arange
    options:
      heading_level: 4
::: pystards.ndarray.full
    options:
      heading_level: 4

## MetadataValue

::: pystards.metadata.MetadataValue
    options:
      heading_level: 3

## Logging

::: pystards.logger
    options:
      heading_level: 3

## Building this page

`pystards` imports the compiled SWIG extension (`_pystards`) at import time, so the
extension must be built and importable before the documentation is built:

```bash
# Build the Python bindings first (see the Installation guide), then serve the
# docs (config lives under docs-site/):
export PYTHONPATH="$PWD/build/bindings/python"
mkdocs serve -f docs-site/mkdocs.yml
```

If the extension is not importable, mkdocstrings cannot introspect the package
and this page will fail to render.

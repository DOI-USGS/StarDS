# stardsls

`stardsls` inspects and lists the contents of a `.stards` file — keys, metadata, and
optionally the array data itself.

Built when `STARDS_BUILD_TOOLS=ON` (the default); see
[Installation](../getting-started/installation.md).

## Usage

```bash
stardsls [OPTIONS] <file.stards>

# List all keys (default)
stardsls data.stards

# Verbose output with metadata
stardsls -v data.stards

# Show a key's metadata without loading its data
stardsls -m array_name data.stards

# Print a specific key's data
stardsls -d array_name data.stards

# Force-load a large array that the safety check would block
stardsls -d array_name -f data.stards

# Print all data (may be large)
stardsls -a data.stards
```

## Options

| Flag | Long form | Description |
|------|-----------|-------------|
| `-h` | `--help` | Show the help message |
| `-k` | `--keys` | List keys only (default) |
| `-d <key>` | `--data <key>` | Print the data for a specific array |
| `-m <key>` | `--metadata <key>` | Show a key's metadata without loading its data |
| `-a` | `--all` | Print the data for all arrays (**warning:** may be large) |
| `-v` | `--verbose` | Verbose output including detailed per-array metadata |
| `-f` | `--force` | Force loading large arrays (bypass the safety check) |

## See also

- [stards_translate](stards-translate.md) — convert between formats.
- [Format Specification](../reference/format-spec.md) — the on-disk layout `stardsls` reads.

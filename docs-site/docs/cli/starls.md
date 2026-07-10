# starls

`starls` inspects and lists the contents of a `.stards` file — keys, metadata, and
optionally the array data itself.

Built when `STAR_BUILD_TOOLS=ON` (the default); see
[Installation](../getting-started/installation.md).

## Usage

```bash
starls [OPTIONS] <file.stards>

# List all keys (default)
starls data.stards

# Verbose output with metadata
starls -v data.stards

# Show a key's metadata without loading its data
starls -m array_name data.stards

# Print a specific key's data
starls -d array_name data.stards

# Force-load a large array that the safety check would block
starls -d array_name -f data.stards

# Print all data (may be large)
starls -a data.stards
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

- [star_translate](star-translate.md) — convert between formats.
- [Format Specification](../reference/format-spec.md) — the on-disk layout `starls` reads.

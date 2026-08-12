"""MkDocs build hooks for the StarDS documentation site.

Two unrelated jobs live here:

1. ``src-dirs`` absolutization for mkdoxy. mkdoxy passes the project's
   ``src-dirs`` to Doxygen as ``INPUT`` verbatim and spawns Doxygen without a
   ``cwd``, so a relative path resolves against whatever directory ``mkdocs``
   was invoked from — not against ``mkdocs.yml``. Building from the repo root
   (as CI does) therefore pointed Doxygen at a nonexistent directory, and every
   C++ API page rendered empty. We rewrite ``src-dirs`` to an absolute path
   anchored at ``mkdocs.yml`` so the build works from any directory, and fail
   loudly if the directory is missing.

2. Post-build sanity check that Doxygen actually found something. Empty XML
   yields empty-but-valid pages, which ``--strict`` cannot see; the check turns
   that silent failure into a build error.

It also suppresses griffe's cosmetic "No type or annotation for parameter/return"
warnings emitted while mkdocstrings introspects the SWIG-backed ``pystards``
package. These aren't actionable — the C++-backed functions legitimately lack
Python type hints — and they would otherwise abort ``mkdocs build --strict``,
which we keep enabled to catch *real* problems such as broken links.

MkDocs counts strict-mode warnings with a ``CountHandler`` attached to the
``mkdocs`` logger, and mkdocstrings routes griffe's messages there. We attach a
filter to that logger (and its handlers) that drops just these annotation
notices, so the strict build still fails on genuine warnings.
"""

import logging
import shlex
import xml.etree.ElementTree as ET
from pathlib import Path

from mkdocs.exceptions import PluginError

log = logging.getLogger("mkdocs")

_NEEDLE = "No type or annotation for"

# Compound kinds that mean "Doxygen parsed real declarations". Namespaces alone
# are not enough — a header that failed to parse can still yield the file entry.
_REAL_COMPOUNDS = frozenset({"class", "struct", "union", "interface", "namespace"})


class _DropAnnotationWarnings(logging.Filter):
    def filter(self, record: logging.LogRecord) -> bool:
        return _NEEDLE not in record.getMessage()


_FILTER = _DropAnnotationWarnings()


def _install() -> None:
    # Attach to the loggers that carry the message and to every handler on the
    # 'mkdocs' logger (which includes MkDocs' strict-mode CountHandler).
    for name in ("mkdocs", "griffe", "mkdocstrings", "mkdocs.plugins.mkdocstrings"):
        logger = logging.getLogger(name)
        if _FILTER not in logger.filters:
            logger.addFilter(_FILTER)
        for handler in logger.handlers:
            if _FILTER not in handler.filters:
                handler.addFilter(_FILTER)


def _mkdoxy_projects(config):
    """Yield (project_name, project_data, plugin_config) for each mkdoxy project."""
    plugin = config.plugins.get("mkdoxy")
    if plugin is None:
        return
    for name, data in (plugin.config.get("projects") or {}).items():
        yield name, data, plugin.config


def _absolutize_mkdoxy_src_dirs(config) -> None:
    # mkdoxy reads 'src-dirs' in on_files, which runs after every on_config
    # hook, so mutating the plugin's config here is picked up.
    root = Path(config.config_file_path).parent.resolve()
    for name, data, _ in _mkdoxy_projects(config):
        src_dirs = data.get("src-dirs")
        if not src_dirs:
            continue
        resolved = []
        for entry in shlex.split(src_dirs):
            path = Path(entry)
            path = path if path.is_absolute() else (root / path)
            path = path.resolve()
            if not path.is_dir():
                raise PluginError(
                    f"mkdoxy project '{name}': src-dirs entry '{entry}' resolves to "
                    f"{path}, which is not a directory. Paths are interpreted relative "
                    f"to {root} (the directory holding mkdocs.yml)."
                )
            resolved.append(str(path))
        data["src-dirs"] = " ".join(resolved)


def _doxygen_xml_index(config, project_name: str, plugin_config) -> Path:
    # Mirrors mkdoxy.plugin.MkDoxy.on_files' tempDir() layout.
    save_api = plugin_config.get("save-api")
    base = Path(save_api) if save_api else Path(config["site_dir"]) / "assets" / ".doxy"
    return base / project_name / "xml" / "index.xml"


def _check_doxygen_found_declarations(config) -> None:
    """Fail the build if Doxygen produced an index with no declarations in it.

    Empty XML renders empty-but-well-formed pages, so `--strict` stays silent —
    exactly the failure mode that shipped an empty Class Index to production.
    """
    for name, _, plugin_config in _mkdoxy_projects(config):
        index = _doxygen_xml_index(config, name, plugin_config)
        if not index.is_file():
            raise PluginError(f"mkdoxy project '{name}': Doxygen wrote no XML index at {index}.")
        kinds = {compound.get("kind") for compound in ET.parse(index).getroot().iter("compound")}
        if not kinds & _REAL_COMPOUNDS:
            raise PluginError(
                f"mkdoxy project '{name}': Doxygen parsed no classes, structs or namespaces "
                f"({index} lists only {sorted(kinds) or 'nothing'}). The generated C++ API pages "
                f"would be empty — check the project's src-dirs, FILE_PATTERNS and PREDEFINED."
            )
        log.info(f"MkDoxy project '{name}': Doxygen index contains {sorted(kinds & _REAL_COMPOUNDS)}")


def on_startup(command, dirty, **kwargs):  # noqa: ARG001
    _install()


def on_config(config, **kwargs):  # noqa: ARG001
    # Re-install after plugins configure their own logging/handlers.
    _install()
    _absolutize_mkdoxy_src_dirs(config)
    return config


def on_post_build(config, **kwargs):  # noqa: ARG001
    _check_doxygen_found_declarations(config)

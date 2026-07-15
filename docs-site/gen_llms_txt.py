"""Generate ``llms.txt`` — the whole StarDS doc site compiled into one Markdown file.

`llms.txt` is a single, plain-Markdown concatenation of every documentation page,
in nav order, intended to be handed to an LLM (or read by a human) as a complete
offline copy of the manual. It is **generated** from the MkDocs ``nav`` — the one
place page order is defined — so it never needs to be maintained by hand.

Two ways it runs:

* As a MkDocs hook (``llms_txt_hook.py`` imports :func:`build` and calls it from
  ``on_post_build``), so ``mkdocs build`` refreshes both the built site's
  ``site/llms.txt`` and the in-repo ``docs/llms.txt`` copy.
* Standalone: ``python gen_llms_txt.py`` regenerates ``docs/llms.txt`` without a
  full site build (handy in CI or a pre-commit check).

Pages pulled from external plugins (mkdoxy's ``StarCPPAPI/*`` and the
mkdocstrings Python API) are generated at build time and are not plain source
Markdown, so they're referenced by URL rather than inlined; everything authored
as Markdown under ``docs/`` is inlined in full.
"""

from __future__ import annotations

import os
import re
from typing import Any

HERE = os.path.dirname(os.path.abspath(__file__))
DOCS_DIR = os.path.join(HERE, "docs")
MKDOCS_YML = os.path.join(HERE, "mkdocs.yml")
OUTPUT_NAME = "llms.txt"

# Front-matter (YAML between leading --- fences) is MkDocs metadata, not content.
_FRONT_MATTER = re.compile(r"\A---\n.*?\n---\n", re.DOTALL)


def _strip_front_matter(text: str) -> str:
    return _FRONT_MATTER.sub("", text, count=1)


def _load_nav() -> list:
    """Read the raw ``nav:`` list from mkdocs.yml.

    We parse with a YAML loader that tolerates MkDocs' ``!!python/name:`` tags
    (used by the emoji extension) so importing PyYAML's plain safe_load doesn't
    choke on them.
    """
    import yaml

    class _Tolerant(yaml.SafeLoader):
        pass

    def _ignore(loader, suffix, node):  # noqa: ARG001
        return None

    # Ignore any custom !!python/... tags that appear in markdown_extensions.
    _Tolerant.add_multi_constructor("tag:yaml.org,2002:python/name:", _ignore)
    _Tolerant.add_multi_constructor("!!python/name:", _ignore)

    with open(MKDOCS_YML, "r", encoding="utf-8") as fh:
        config = yaml.load(fh, Loader=_Tolerant)
    return config.get("nav", []) or []


def _flatten_nav(nav: Any, out: list[tuple[list[str], str]], trail: list[str]) -> None:
    """Walk the nav tree, collecting (section_trail, target) leaves in order.

    ``target`` is either a doc-relative markdown path (e.g. ``guides/layers.md``)
    or an external/URL string. Section titles form the ``trail`` for headings.
    """
    if isinstance(nav, str):
        out.append((list(trail), nav))
        return
    if isinstance(nav, list):
        for item in nav:
            _flatten_nav(item, out, trail)
        return
    if isinstance(nav, dict):
        for title, value in nav.items():
            if isinstance(value, (list, dict)):
                _flatten_nav(value, out, trail + [str(title)])
            else:
                # Leaf: {Title: target}
                out.append((trail + [str(title)], str(value)))


def build(site_dir: str | None = None) -> str:
    """Generate llms.txt. Returns the generated text.

    Always writes ``docs/llms.txt`` (so the file is committable / servable as a
    normal doc). If ``site_dir`` is given (a real MkDocs build), also writes
    ``<site_dir>/llms.txt`` so it's published at ``/llms.txt``.
    """
    nav = _load_nav()
    leaves: list[tuple[list[str], str]] = []
    _flatten_nav(nav, leaves, [])

    site_url = "https://code.usgs.gov/astrogeology/stards"
    parts: list[str] = []
    parts.append("# StarDS — Full Documentation (llms.txt)\n")
    parts.append(
        "> This file is the entire StarDS manual concatenated into one Markdown "
        "document, generated automatically from the MkDocs navigation. It is meant "
        "for LLMs and offline reading. Sections appear in site-navigation order.\n"
    )

    seen_files: set[str] = set()
    external: list[tuple[list[str], str]] = []

    for trail, target in leaves:
        # External links (http...) and generated API trees aren't plain source
        # Markdown; list them at the end as references instead of inlining.
        is_url = target.startswith("http://") or target.startswith("https://")
        md_path = os.path.join(DOCS_DIR, target)
        if is_url or not target.endswith(".md") or not os.path.isfile(md_path):
            external.append((trail, target))
            continue
        if target in seen_files:
            continue
        seen_files.add(target)

        with open(md_path, "r", encoding="utf-8") as fh:
            body = _strip_front_matter(fh.read()).strip()

        section = " / ".join(trail) if trail else target
        parts.append("\n\n---\n")
        parts.append(f"<!-- source: docs/{target} -->")
        parts.append(f"\n# {section}\n")
        parts.append(body)

    if external:
        parts.append("\n\n---\n")
        parts.append("\n# Generated / external references\n")
        parts.append(
            "These pages are generated at build time (C++ API via mkdoxy, Python "
            "API via mkdocstrings) or link off-site, so they are referenced rather "
            "than inlined:\n"
        )
        for trail, target in external:
            label = " / ".join(trail) if trail else target
            if target.startswith("http"):
                parts.append(f"- [{label}]({target})")
            else:
                url = f"{site_url}/-/blob/main/docs-site/docs/{target}"
                parts.append(f"- {label} — `{target}` ({url})")

    text = "\n".join(parts).rstrip() + "\n"

    # Always refresh the in-repo copy.
    with open(os.path.join(DOCS_DIR, OUTPUT_NAME), "w", encoding="utf-8") as fh:
        fh.write(text)

    # If building the site, also emit it into the published output.
    if site_dir:
        with open(os.path.join(site_dir, OUTPUT_NAME), "w", encoding="utf-8") as fh:
            fh.write(text)

    return text


if __name__ == "__main__":
    out = build()
    print(f"Wrote {os.path.join(DOCS_DIR, OUTPUT_NAME)} ({len(out)} bytes)")

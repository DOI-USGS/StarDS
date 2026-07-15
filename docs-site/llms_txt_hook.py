"""MkDocs hook: regenerate ``llms.txt`` on every build.

Delegates to :func:`gen_llms_txt.build`, which compiles the whole doc site into a
single Markdown file (see that module's docstring). Running it from
``on_post_build`` — after the site is rendered — lets us drop the file straight
into the published ``site/`` directory (served at ``/llms.txt``) and refresh the
in-repo ``docs/llms.txt`` copy at the same time.
"""

from __future__ import annotations

import importlib.util
import logging
import os

_HERE = os.path.dirname(os.path.abspath(__file__))


def _load_generator():
    # MkDocs loads hooks as standalone modules and does NOT add their directory
    # to sys.path, so a plain `import gen_llms_txt` fails. Load it by explicit
    # path from this file's directory instead.
    path = os.path.join(_HERE, "gen_llms_txt.py")
    spec = importlib.util.spec_from_file_location("stards_gen_llms_txt", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def on_post_build(config, **kwargs):  # noqa: ARG001
    gen = _load_generator()
    site_dir = config.get("site_dir") if hasattr(config, "get") else config["site_dir"]
    text = gen.build(site_dir=site_dir)
    logging.getLogger("mkdocs").info(
        "llms.txt generated (%d bytes) -> %s/llms.txt", len(text), site_dir
    )
    return None

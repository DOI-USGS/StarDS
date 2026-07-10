"""MkDocs build hooks for the StarDS documentation site.

Suppresses griffe's cosmetic "No type or annotation for parameter/return"
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

_NEEDLE = "No type or annotation for"


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


def on_startup(command, dirty, **kwargs):  # noqa: ARG001
    _install()


def on_config(config, **kwargs):  # noqa: ARG001
    # Re-install after plugins configure their own logging/handlers.
    _install()
    return config

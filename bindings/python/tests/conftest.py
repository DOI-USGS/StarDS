import pytest
import tempfile
import os
import sys
import importlib.util

# Make the built extension importable regardless of the working directory, but
# only if it isn't already importable (e.g. via PYTHONPATH). This avoids
# shadowing a caller-provided build with a possibly-stale in-tree one.
if importlib.util.find_spec("pystards") is None:
    _repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
    for _candidate in (
        os.environ.get("PYSTARDS_BUILD_DIR"),
        os.path.join(_repo_root, "build", "bindings", "python"),
    ):
        if _candidate and os.path.isdir(_candidate):
            sys.path.insert(0, _candidate)
            break

from pystards import StarDataset


@pytest.fixture
def temp_star_file():
    """Create temporary STAR file path (file created by StarDataset)"""
    fd, path = tempfile.mkstemp(suffix='.stards')
    os.close(fd)
    # Remove the file so StarDataset can create it fresh
    os.unlink(path)
    yield path
    # Clean up after test
    if os.path.exists(path):
        os.unlink(path)


@pytest.fixture
def store(temp_star_file):
    """Create StarDataset instance"""
    return StarDataset(temp_star_file)

import pytest
import tempfile
import os
from pystar import StarDataset


@pytest.fixture
def temp_star_file():
    """Create temporary STAR file path (file created by StarDataset)"""
    fd, path = tempfile.mkstemp(suffix='.star')
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

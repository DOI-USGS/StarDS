"""
STARDS Python Bindings

STARDS (Simple Tensors Arrays and Rasters Data Store) for persistent
N-dimensional arrays.
"""

from ._version import __version__
from .dataset import StarDataset
from .ndarray import NDArray, zeros, ones, arange, full
from .metadata import MetadataValue
from .enums import DataType, CompressionAlgorithm, FileMode
from .logger import LogLevel, set_log_level, get_log_level

# Import from SWIG module
try:
    from . import pystards as _star
except ImportError:
    import pystards as _star

# Expose getLibraryVersion at module level
get_library_version = _star.get_library_version

# Expose FileHeader class
FileHeader = _star.FileHeader

# Expose StarConfig class
StarConfig = _star.StarConfig

# Expose OpenOptions class (read-time open options, e.g. layer_inheritance)
OpenOptions = _star.OpenOptions

__all__ = [
    '__version__',
    'StarDataset',
    'NDArray',
    'MetadataValue',
    'DataType',
    'CompressionAlgorithm',
    'FileMode',
    'LogLevel',
    'set_log_level',
    'get_log_level',
    'zeros',
    'ones',
    'arange',
    'full',
    'get_library_version',
    'FileHeader',
    'StarConfig',
    'OpenOptions',
]

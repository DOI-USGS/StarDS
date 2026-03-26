"""Enum types for STAR"""
try:
    from . import pystar as _star
except ImportError:
    import pystar as _star


# Create enum-like classes for better API
class DataType:
    INT8 = _star.DataType_INT8
    INT16 = _star.DataType_INT16
    INT32 = _star.DataType_INT32
    INT64 = _star.DataType_INT64
    UINT8 = _star.DataType_UINT8
    UINT16 = _star.DataType_UINT16
    UINT32 = _star.DataType_UINT32
    UINT64 = _star.DataType_UINT64
    FLOAT32 = _star.DataType_FLOAT32
    FLOAT64 = _star.DataType_FLOAT64
    STRING = _star.DataType_STRING


class CompressionAlgorithm:
    NONE = _star.CompressionAlgorithm_NONE
    GZIP = _star.CompressionAlgorithm_GZIP
    ZSTD = _star.CompressionAlgorithm_ZSTD
    LZ4 = _star.CompressionAlgorithm_LZ4


class FileMode:
    READ_WRITE = _star.FileMode_READ_WRITE
    READ_ONLY = _star.FileMode_READ_ONLY


__all__ = ['DataType', 'CompressionAlgorithm', 'FileMode']

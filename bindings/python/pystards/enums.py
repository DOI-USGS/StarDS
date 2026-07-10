"""Enum types for STAR"""
try:
    from . import pystards as _star
except ImportError:
    import pystards as _star


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
    # Byte-shuffle prefilter + base codec (better ratios on numeric arrays).
    GZIP_SHUFFLE = _star.CompressionAlgorithm_GZIP_SHUFFLE
    LZ4_SHUFFLE = _star.CompressionAlgorithm_LZ4_SHUFFLE


class FileMode:
    READ_WRITE = _star.FileMode_READ_WRITE
    READ_ONLY = _star.FileMode_READ_ONLY


__all__ = ['DataType', 'CompressionAlgorithm', 'FileMode']

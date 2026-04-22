"""Logger configuration for STARDS"""
try:
    from . import pystar as _star
except ImportError:
    import pystar as _star


class LogLevel:
    """Log level constants"""
    TRACE = 0
    DEBUG = 1
    INFO = 2
    WARN = 3
    ERROR = 4


def set_log_level(level):
    """
    Set the STARDS logging level at runtime.

    Args:
        level: Log level (use LogLevel.TRACE, .DEBUG, .INFO, .WARN, or .ERROR)
               or integer 0-4

    Examples:
        >>> import pystar
        >>> pystar.set_log_level(pystar.LogLevel.DEBUG)
        >>> # Now debug messages will be printed

        >>> # Or use integer directly
        >>> pystar.set_log_level(1)  # DEBUG
    """
    if isinstance(level, str):
        level_map = {
            'TRACE': 0, 'trace': 0,
            'DEBUG': 1, 'debug': 1,
            'INFO': 2, 'info': 2,
            'WARN': 3, 'warn': 3, 'WARNING': 3, 'warning': 3,
            'ERROR': 4, 'error': 4,
        }
        if level not in level_map:
            raise ValueError(f"Invalid log level: {level}. Use TRACE, DEBUG, INFO, WARN, or ERROR")
        level = level_map[level]

    if not isinstance(level, int) or level < 0 or level > 4:
        raise ValueError(f"Log level must be integer 0-4 or string, got: {level}")

    # Access the logger class from the SWIG-generated module
    _star.logger.set_log_level(int(level))


def get_log_level():
    """
    Get the current STARDS logging level.

    Returns:
        int: Current log level (0=TRACE, 1=DEBUG, 2=INFO, 3=WARN, 4=ERROR)

    Examples:
        >>> import pystar
        >>> level = pystar.get_log_level()
        >>> print(f"Current log level: {level}")
    """
    # Access the logger class from the SWIG-generated module
    return int(_star.logger.get_log_level())


__all__ = ['LogLevel', 'set_log_level', 'get_log_level']

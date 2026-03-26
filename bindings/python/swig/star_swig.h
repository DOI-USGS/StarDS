// SWIG-friendly wrapper for star.h
// This header preprocesses star.h to hide SFINAE constructs

#ifndef SWIG
// If not SWIG, just include normally
#include "star.h"
#else
// For SWIG: define macros to hide enable_if
#define SWIG_PREPROCESSING 1

// Include the main header
#include "star.h"

#endif  // SWIG

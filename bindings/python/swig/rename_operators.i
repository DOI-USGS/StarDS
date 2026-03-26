// Rename C++ operators to Python-friendly names
%rename(__call__) NDArray::operator();
%rename(__len__) NDArray::size;

// Rename 'print' which is a Python keyword
%rename(print_info) print;

// Ignore C++ iterators (we'll provide Python iteration)
%ignore NDArray::begin;
%ignore NDArray::end;
%ignore NDArray::cbegin;
%ignore NDArray::cend;
%ignore NDArray::rbegin;
%ignore NDArray::rend;

// Ignore internal/deprecated methods
%ignore NDArray::at;  // Use operator() instead

// Ignore complex SFINAE template methods that SWIG can't parse
%ignore NDArray::arange;
%ignore NDArray::zeros;
%ignore NDArray::ones;
%ignore NDArray::full;

// Ignore nested structs (SWIG limitation)
%ignore ExtractionPlan::ElementRange;

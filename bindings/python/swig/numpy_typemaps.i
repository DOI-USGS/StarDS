// Typemaps for fixed-width integer types
// SWIG's std_vector.i works for float/double but not for int8_t, int16_t, etc.
// These typemaps allow Python int to convert to C++ fixed-width integers

%typemap(in) int8_t {
    $1 = (int8_t)PyLong_AsLong($input);
}

%typemap(in) int16_t {
    $1 = (int16_t)PyLong_AsLong($input);
}

%typemap(in) int32_t {
    $1 = (int32_t)PyLong_AsLong($input);
}

%typemap(in) int64_t {
    $1 = (int64_t)PyLong_AsLongLong($input);
}

%typemap(in) uint8_t {
    $1 = (uint8_t)PyLong_AsUnsignedLong($input);
}

%typemap(in) uint16_t {
    $1 = (uint16_t)PyLong_AsUnsignedLong($input);
}

%typemap(in) uint32_t {
    $1 = (uint32_t)PyLong_AsUnsignedLong($input);
}

%typemap(in) uint64_t {
    $1 = (uint64_t)PyLong_AsUnsignedLongLong($input);
}

// Typemaps for const references (needed for vector::append, etc.)
%typemap(in) const int8_t& (int8_t temp) {
    temp = (int8_t)PyLong_AsLong($input);
    $1 = &temp;
}

%typemap(in) const int16_t& (int16_t temp) {
    temp = (int16_t)PyLong_AsLong($input);
    $1 = &temp;
}

%typemap(in) const int32_t& (int32_t temp) {
    temp = (int32_t)PyLong_AsLong($input);
    $1 = &temp;
}

%typemap(in) const int64_t& (int64_t temp) {
    temp = (int64_t)PyLong_AsLongLong($input);
    $1 = &temp;
}

%typemap(in) const uint8_t& (uint8_t temp) {
    temp = (uint8_t)PyLong_AsUnsignedLong($input);
    $1 = &temp;
}

%typemap(in) const uint16_t& (uint16_t temp) {
    temp = (uint16_t)PyLong_AsUnsignedLong($input);
    $1 = &temp;
}

%typemap(in) const uint32_t& (uint32_t temp) {
    temp = (uint32_t)PyLong_AsUnsignedLong($input);
    $1 = &temp;
}

%typemap(in) const uint64_t& (uint64_t temp) {
    temp = (uint64_t)PyLong_AsUnsignedLongLong($input);
    $1 = &temp;
}

// Output typemaps to return Python int
%typemap(out) int8_t {
    $result = PyLong_FromLong($1);
}

%typemap(out) int16_t {
    $result = PyLong_FromLong($1);
}

%typemap(out) int32_t {
    $result = PyLong_FromLong($1);
}

%typemap(out) int64_t {
    $result = PyLong_FromLongLong($1);
}

%typemap(out) uint8_t {
    $result = PyLong_FromUnsignedLong($1);
}

%typemap(out) uint16_t {
    $result = PyLong_FromUnsignedLong($1);
}

%typemap(out) uint32_t {
    $result = PyLong_FromUnsignedLong($1);
}

%typemap(out) uint64_t {
    $result = PyLong_FromUnsignedLongLong($1);
}

// Typemaps for std::vector<T>::value_type const & (used by std_vector.i)
%typemap(in) std::vector<int8_t>::value_type const & (int8_t temp) {
    temp = (int8_t)PyLong_AsLong($input);
    $1 = &temp;
}

%typemap(in) std::vector<int16_t>::value_type const & (int16_t temp) {
    temp = (int16_t)PyLong_AsLong($input);
    $1 = &temp;
}

%typemap(in) std::vector<int32_t>::value_type const & (int32_t temp) {
    temp = (int32_t)PyLong_AsLong($input);
    $1 = &temp;
}

%typemap(in) std::vector<int64_t>::value_type const & (int64_t temp) {
    temp = (int64_t)PyLong_AsLongLong($input);
    $1 = &temp;
}

%typemap(in) std::vector<uint8_t>::value_type const & (uint8_t temp) {
    temp = (uint8_t)PyLong_AsUnsignedLong($input);
    $1 = &temp;
}

%typemap(in) std::vector<uint16_t>::value_type const & (uint16_t temp) {
    temp = (uint16_t)PyLong_AsUnsignedLong($input);
    $1 = &temp;
}

%typemap(in) std::vector<uint32_t>::value_type const & (uint32_t temp) {
    temp = (uint32_t)PyLong_AsUnsignedLong($input);
    $1 = &temp;
}

%typemap(in) std::vector<uint64_t>::value_type const & (uint64_t temp) {
    temp = (uint64_t)PyLong_AsUnsignedLongLong($input);
    $1 = &temp;
}

// Typemaps for pointer/reference returns (for operator[] and flat())
%typemap(out) int8_t& {
    $result = PyLong_FromLong(*$1);
}

%typemap(out) int16_t& {
    $result = PyLong_FromLong(*$1);
}

%typemap(out) int32_t& {
    $result = PyLong_FromLong(*$1);
}

%typemap(out) int64_t& {
    $result = PyLong_FromLongLong(*$1);
}

%typemap(out) uint8_t& {
    $result = PyLong_FromUnsignedLong(*$1);
}

%typemap(out) uint16_t& {
    $result = PyLong_FromUnsignedLong(*$1);
}

%typemap(out) uint32_t& {
    $result = PyLong_FromUnsignedLong(*$1);
}

%typemap(out) uint64_t& {
    $result = PyLong_FromUnsignedLongLong(*$1);
}
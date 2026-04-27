using namespace star;

// Instantiate NDArray for all supported types
%template(NDArrayInt8)    NDArray<int8_t>;
%template(NDArrayInt16)   NDArray<int16_t>;
%template(NDArrayInt32)   NDArray<int32_t>;
%template(NDArrayInt64)   NDArray<int64_t>;
%template(NDArrayUInt8)   NDArray<uint8_t>;
%template(NDArrayUInt16)  NDArray<uint16_t>;
%template(NDArrayUInt32)  NDArray<uint32_t>;
%template(NDArrayUInt64)  NDArray<uint64_t>;
%template(NDArrayFloat32) NDArray<float>;
%template(NDArrayFloat64) NDArray<double>;
%template(NDArrayString)  NDArray<std::string>;

// Instantiate get for all types (retrieve data arrays, not metadata)
%template(get_int8)    StarDataset::get<int8_t>;
%template(get_int16)   StarDataset::get<int16_t>;
%template(get_int32)   StarDataset::get<int32_t>;
%template(get_int64)   StarDataset::get<int64_t>;
%template(get_uint8)   StarDataset::get<uint8_t>;
%template(get_uint16)  StarDataset::get<uint16_t>;
%template(get_uint32)  StarDataset::get<uint32_t>;
%template(get_uint64)  StarDataset::get<uint64_t>;
%template(get_float32) StarDataset::get<float>;
%template(get_float64) StarDataset::get<double>;
%template(get_string)  StarDataset::get<std::string>;

// Instantiate get_slice for numeric types
%template(get_slice_int8)    StarDataset::get_slice<int8_t>;
%template(get_slice_int16)   StarDataset::get_slice<int16_t>;
%template(get_slice_int32)   StarDataset::get_slice<int32_t>;
%template(get_slice_int64)   StarDataset::get_slice<int64_t>;
%template(get_slice_uint8)   StarDataset::get_slice<uint8_t>;
%template(get_slice_uint16)  StarDataset::get_slice<uint16_t>;
%template(get_slice_uint32)  StarDataset::get_slice<uint32_t>;
%template(get_slice_uint64)  StarDataset::get_slice<uint64_t>;
%template(get_slice_float32) StarDataset::get_slice<float>;
%template(get_slice_float64) StarDataset::get_slice<double>;

// Instantiate MetadataAccessor::put for all types
%template(put_int8)    MetadataAccessor::put<NDArray<int8_t>>;
%template(put_int16)   MetadataAccessor::put<NDArray<int16_t>>;
%template(put_int32)   MetadataAccessor::put<NDArray<int32_t>>;
%template(put_int64)   MetadataAccessor::put<NDArray<int64_t>>;
%template(put_uint8)   MetadataAccessor::put<NDArray<uint8_t>>;
%template(put_uint16)  MetadataAccessor::put<NDArray<uint16_t>>;
%template(put_uint32)  MetadataAccessor::put<NDArray<uint32_t>>;
%template(put_uint64)  MetadataAccessor::put<NDArray<uint64_t>>;
%template(put_float32) MetadataAccessor::put<NDArray<float>>;
%template(put_float64) MetadataAccessor::put<NDArray<double>>;
%template(put_string)  MetadataAccessor::put<NDArray<std::string>>;

// Instantiate StarDataset::put for all types (stores separately, not in metadata block)
%template(put_array_int8)    StarDataset::put<int8_t>;
%template(put_array_int16)   StarDataset::put<int16_t>;
%template(put_array_int32)   StarDataset::put<int32_t>;
%template(put_array_int64)   StarDataset::put<int64_t>;
%template(put_array_uint8)   StarDataset::put<uint8_t>;
%template(put_array_uint16)  StarDataset::put<uint16_t>;
%template(put_array_uint32)  StarDataset::put<uint32_t>;
%template(put_array_uint64)  StarDataset::put<uint64_t>;
%template(put_array_float32) StarDataset::put<float>;
%template(put_array_float64) StarDataset::put<double>;
%template(put_array_string)  StarDataset::put<std::string>;

// Instantiate std::map for batch operations
%template(MapStringNDArrayInt64) std::map<std::string, NDArray<int64_t>>;
%template(MapStringNDArrayFloat64) std::map<std::string, NDArray<double>>;
%template(MapStringNDArrayString) std::map<std::string, NDArray<std::string>>;
%template(MapStringMetadataValue) std::map<std::string, MetadataValue>;

// Instantiate MetadataAccessor::put_batch for all types
%template(put_batch_int8)    MetadataAccessor::put_batch<NDArray<int8_t>>;
%template(put_batch_int16)   MetadataAccessor::put_batch<NDArray<int16_t>>;
%template(put_batch_int32)   MetadataAccessor::put_batch<NDArray<int32_t>>;
%template(put_batch_int64)   MetadataAccessor::put_batch<NDArray<int64_t>>;
%template(put_batch_uint8)   MetadataAccessor::put_batch<NDArray<uint8_t>>;
%template(put_batch_uint16)  MetadataAccessor::put_batch<NDArray<uint16_t>>;
%template(put_batch_uint32)  MetadataAccessor::put_batch<NDArray<uint32_t>>;
%template(put_batch_uint64)  MetadataAccessor::put_batch<NDArray<uint64_t>>;
%template(put_batch_float32) MetadataAccessor::put_batch<NDArray<float>>;
%template(put_batch_float64) MetadataAccessor::put_batch<NDArray<double>>;
%template(put_batch_string)  MetadataAccessor::put_batch<NDArray<std::string>>;

// Instantiate LayerView::get for all types
%template(get_int8)    LayerView::get<int8_t>;
%template(get_int16)   LayerView::get<int16_t>;
%template(get_int32)   LayerView::get<int32_t>;
%template(get_int64)   LayerView::get<int64_t>;
%template(get_uint8)   LayerView::get<uint8_t>;
%template(get_uint16)  LayerView::get<uint16_t>;
%template(get_uint32)  LayerView::get<uint32_t>;
%template(get_uint64)  LayerView::get<uint64_t>;
%template(get_float32) LayerView::get<float>;
%template(get_float64) LayerView::get<double>;
%template(get_string)  LayerView::get<std::string>;

// Instantiate LayerView::put for all types
%template(put_int8)    LayerView::put<int8_t>;
%template(put_int16)   LayerView::put<int16_t>;
%template(put_int32)   LayerView::put<int32_t>;
%template(put_int64)   LayerView::put<int64_t>;
%template(put_uint8)   LayerView::put<uint8_t>;
%template(put_uint16)  LayerView::put<uint16_t>;
%template(put_uint32)  LayerView::put<uint32_t>;
%template(put_uint64)  LayerView::put<uint64_t>;
%template(put_float32) LayerView::put<float>;
%template(put_float64) LayerView::put<double>;
%template(put_string)  LayerView::put<std::string>;


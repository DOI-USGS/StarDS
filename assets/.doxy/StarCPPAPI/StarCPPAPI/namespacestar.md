

# Namespace star



[**Namespace List**](namespaces.md) **>** [**star**](namespacestar.md)


















## Namespaces

| Type | Name |
| ---: | :--- |
| namespace | [**logger**](namespacestar_1_1logger.md) <br> |
| namespace | [**s3crypto**](namespacestar_1_1s3crypto.md) <br> |
| namespace | [**shuffle\_detail**](namespacestar_1_1shuffle__detail.md) <br> |


## Classes

| Type | Name |
| ---: | :--- |
| class | [**AWSConfigParser**](classstar_1_1AWSConfigParser.md) <br>_AWS Configuration file parser._  |
| class | [**AWSTokenCache**](classstar_1_1AWSTokenCache.md) <br>_AWS SSO Token Cache reader._  |
| class | [**AWSV4Signer**](classstar_1_1AWSV4Signer.md) <br>_AWS Signature Version 4 signer._  |
| struct | [**BlockInfo**](structstar_1_1BlockInfo.md) <br>_Metadata for a single compressed block._  |
| struct | [**BlockMap**](structstar_1_1BlockMap.md) <br>_Maps logical elements to physical blocks._  |
| struct | [**ByteUnshuffleSpec**](structstar_1_1ByteUnshuffleSpec.md) <br>_Describes an in-place byte-unshuffle to fuse into an array's fill._  |
| struct | [**ColdStorage**](structstar_1_1ColdStorage.md) <br>_Cold storage - infrequently accessed data._  |
| struct | [**ExtractionPlan**](structstar_1_1ExtractionPlan.md) <br>_Describes how to extract elements from blocks._  |
| struct | [**FileHeader**](structstar_1_1FileHeader.md) <br>_File header structure (31 bytes fixed size)._  |
| struct | [**FilePathInfo**](structstar_1_1FilePathInfo.md) <br> |
| struct | [**HotStorage**](structstar_1_1HotStorage.md) <br>_Hot storage - frequently accessed data (cache-friendly)._  |
| class | [**HttpRangeReader**](classstar_1_1HttpRangeReader.md) <br> |
| struct | [**IndexEntry**](structstar_1_1IndexEntry.md) <br>_Index entry with block compression support and shape information._  |
| struct | [**KeyRegistry**](structstar_1_1KeyRegistry.md) <br>_Global key registry using data-oriented design (Structure of Arrays)._  |
| class | [**LayerMetadataAccessor**](classstar_1_1LayerMetadataAccessor.md) <br>_Metadata accessor for a specific layer with inheritance._  |
| struct | [**LayerMetadataRegistry**](structstar_1_1LayerMetadataRegistry.md) <br>_Per-layer metadata registry using data-oriented design (Structure of Arrays)._  |
| class | [**LayerView**](classstar_1_1LayerView.md) <br>_Lightweight view into a specific layer with inheritance from base._  |
| class | [**LocalRangeReader**](classstar_1_1LocalRangeReader.md) <br> |
| class | [**MemoryRangeReader**](classstar_1_1MemoryRangeReader.md) <br> |
| class | [**MetadataAccessor**](classstar_1_1MetadataAccessor.md) <br>_Accessor for metadata operations._  |
| struct | [**MetadataValue**](structstar_1_1MetadataValue.md) <br>_Type-erased wrapper for metadata values._  |
| class | [**NDArray**](classstar_1_1NDArray.md) &lt;typename T&gt;<br>_Modern n-dimensional array class with xtensor-style API._  |
| struct | [**OpenOptions**](structstar_1_1OpenOptions.md) <br>_Read-time options for_ [_**StarDataset::open()**_](classstar_1_1StarDataset.md#function-open-12) _._ |
| class | [**RangeReader**](classstar_1_1RangeReader.md) <br> |
| struct | [**S3Credentials**](structstar_1_1S3Credentials.md) <br>_AWS Credentials with resolution chain._  |
| struct | [**S3EndpointConfig**](structstar_1_1S3EndpointConfig.md) <br>_S3 endpoint resolution (default AWS, or an override for S3-compatible services such as MinIO and for local testing)._  |
| class | [**S3RangeReader**](classstar_1_1S3RangeReader.md) <br> |
| class | [**S3Writer**](classstar_1_1S3Writer.md) <br>_S3 writer for uploading objects._  |
| struct | [**Slice**](structstar_1_1Slice.md) <br>_Describes a slice along one dimension (Python-style slicing) Plain struct - no methods except helpers, just data._  |
| struct | [**SliceSpec**](structstar_1_1SliceSpec.md) <br>_Complete slice specification for n-dimensional array._  |
| struct | [**StarConfig**](structstar_1_1StarConfig.md) <br>_Configuration for metadata block optimization._  |
| class | [**StarDataset**](classstar_1_1StarDataset.md) <br>_A cloud-optimized binary key-value store for serializable data types._  |
| class | [**ThreadPool**](classstar_1_1ThreadPool.md) <br>_Simple thread pool for parallel block operations._  |
| struct | [**TypeToDataType**](structstar_1_1TypeToDataType.md) &lt;typename T&gt;<br> |
| struct | [**TypeToDataType&lt; double &gt;**](structstar_1_1TypeToDataType_3_01double_01_4.md) &lt;&gt;<br> |
| struct | [**TypeToDataType&lt; float &gt;**](structstar_1_1TypeToDataType_3_01float_01_4.md) &lt;&gt;<br> |
| struct | [**TypeToDataType&lt; int16\_t &gt;**](structstar_1_1TypeToDataType_3_01int16__t_01_4.md) &lt;&gt;<br> |
| struct | [**TypeToDataType&lt; int32\_t &gt;**](structstar_1_1TypeToDataType_3_01int32__t_01_4.md) &lt;&gt;<br> |
| struct | [**TypeToDataType&lt; int64\_t &gt;**](structstar_1_1TypeToDataType_3_01int64__t_01_4.md) &lt;&gt;<br> |
| struct | [**TypeToDataType&lt; int8\_t &gt;**](structstar_1_1TypeToDataType_3_01int8__t_01_4.md) &lt;&gt;<br> |
| struct | [**TypeToDataType&lt; std::string &gt;**](structstar_1_1TypeToDataType_3_01std_1_1string_01_4.md) &lt;&gt;<br> |
| struct | [**TypeToDataType&lt; uint16\_t &gt;**](structstar_1_1TypeToDataType_3_01uint16__t_01_4.md) &lt;&gt;<br> |
| struct | [**TypeToDataType&lt; uint32\_t &gt;**](structstar_1_1TypeToDataType_3_01uint32__t_01_4.md) &lt;&gt;<br> |
| struct | [**TypeToDataType&lt; uint64\_t &gt;**](structstar_1_1TypeToDataType_3_01uint64__t_01_4.md) &lt;&gt;<br> |
| struct | [**TypeToDataType&lt; uint8\_t &gt;**](structstar_1_1TypeToDataType_3_01uint8__t_01_4.md) &lt;&gt;<br> |


## Public Types

| Type | Name |
| ---: | :--- |
| enum uint8\_t | [**CompressionAlgorithm**](#enum-compressionalgorithm)  <br> |
| enum uint8\_t | [**DataType**](#enum-datatype)  <br> |
| enum  | [**FileMode**](#enum-filemode)  <br> |
| enum  | [**StorageClass**](#enum-storageclass)  <br>_Storage classification for values (DEPRECATED - use StorageLocation)._  |
| enum  | [**StorageLocation**](#enum-storagelocation)  <br>_Storage location state for unified storage model._  |
| typedef std::variant&lt; [**NDArray**](classstar_1_1NDArray.md)&lt; int8\_t &gt;, [**NDArray**](classstar_1_1NDArray.md)&lt; int16\_t &gt;, [**NDArray**](classstar_1_1NDArray.md)&lt; int32\_t &gt;, [**NDArray**](classstar_1_1NDArray.md)&lt; int64\_t &gt;, [**NDArray**](classstar_1_1NDArray.md)&lt; uint8\_t &gt;, [**NDArray**](classstar_1_1NDArray.md)&lt; uint16\_t &gt;, [**NDArray**](classstar_1_1NDArray.md)&lt; uint32\_t &gt;, [**NDArray**](classstar_1_1NDArray.md)&lt; uint64\_t &gt;, [**NDArray**](classstar_1_1NDArray.md)&lt; float &gt;, [**NDArray**](classstar_1_1NDArray.md)&lt; double &gt;, [**NDArray**](classstar_1_1NDArray.md)&lt; std::string &gt; &gt; | [**ValueVariant**](#typedef-valuevariant)  <br> |




## Public Attributes

| Type | Name |
| ---: | :--- |
|  const char \* | [**MAGIC\_STRING**](#variable-magic_string)   = `"STARDS"`<br> |
|  const size\_t | [**MAGIC\_STRING\_LENGTH**](#variable-magic_string_length)   = `6`<br> |
|  const std::string | [**PROJECT\_NAME**](#variable-project_name)   = `"STARDS " STAR\_VERSION\_STRING`<br> |
|  size\_t | [**g\_min\_blocks\_for\_threading**](#variable-g_min_blocks_for_threading)   = `4`<br> |
|  size\_t | [**g\_min\_bytes\_for\_threading**](#variable-g_min_bytes_for_threading)   = `256 \* 1024`<br> |
|  std::atomic&lt; uint64\_t &gt; | [**g\_network\_request\_count**](#variable-g_network_request_count)   = `{0}`<br> |
|  size\_t | [**g\_num\_threads**](#variable-g_num_threads)   = `0`<br> |
















## Public Functions

| Type | Name |
| ---: | :--- |
|  CompressionAlgorithm | [**base\_compression**](#function-base_compression) (CompressionAlgorithm c) <br>_The underlying block codec for a (possibly shuffle-prefiltered) algorithm._  |
|  void | [**byte\_shuffle**](#function-byte_shuffle) (const char \* in, char \* out, size\_t count, size\_t elem\_size) <br>_Byte-shuffle: reorder_ `count` _elements of_`elem_size` _bytes so that byte plane 0 of every element comes first, then plane 1, etc._ |
|  void | [**byte\_shuffle\_blocked**](#function-byte_shuffle_blocked) (const char \* in, char \* out, size\_t data\_size, size\_t elem\_size, size\_t block\_size) <br>_Per-block byte-shuffle: apply byte\_shuffle() independently within each_ `block_size` _-byte chunk of a_`data_size` _-byte buffer._ |
|  void | [**byte\_unshuffle**](#function-byte_unshuffle) (const char \* in, char \* out, size\_t count, size\_t elem\_size) <br>_Inverse of byte\_shuffle(): reassemble byte planes back into elements._  |
|  void | [**byte\_unshuffle\_blocked**](#function-byte_unshuffle_blocked) (const char \* in, char \* out, size\_t data\_size, size\_t elem\_size, size\_t block\_size) <br>_Inverse of byte\_shuffle\_blocked(): un-shuffle each_ `block_size` _chunk._ |
|  std::pair&lt; std::vector&lt; char &gt;, std::vector&lt; [**BlockInfo**](structstar_1_1BlockInfo.md) &gt; &gt; | [**compressBlocks**](#function-compressblocks) (const char \* data, size\_t data\_size, CompressionAlgorithm algorithm, size\_t block\_size) <br>_Compresses data in blocks and returns block metadata (legacy version)._  |
|  std::vector&lt; [**BlockInfo**](structstar_1_1BlockInfo.md) &gt; | [**compressBlocksBuffered**](#function-compressblocksbuffered) (const char \* data, size\_t data\_size, CompressionAlgorithm algorithm, size\_t block\_size, std::vector&lt; char &gt; & compressed\_output, std::vector&lt; char &gt; & temp\_buffer, [**ThreadPool**](classstar_1_1ThreadPool.md) \* thread\_pool=nullptr) <br>_Compresses data in blocks using pre-allocated buffer._  |
|  size\_t | [**datatype\_size**](#function-datatype_size) (DataType dtype) <br>_Get element size in bytes for a DataType._  |
|  const char \* | [**datatype\_to\_string**](#function-datatype_to_string) (DataType dtype) <br>_Get string representation of DataType._  |
|  std::vector&lt; char &gt; | [**decompressBlocks**](#function-decompressblocks) (const std::vector&lt; char &gt; & compressed\_data, const std::vector&lt; [**BlockInfo**](structstar_1_1BlockInfo.md) &gt; & blocks, CompressionAlgorithm algorithm, const std::vector&lt; size\_t &gt; & block\_indices={}, [**ThreadPool**](classstar_1_1ThreadPool.md) \* thread\_pool=nullptr) <br>_Decompresses specific blocks from compressed data._  |
|  void | [**decompressBlocksInto**](#function-decompressblocksinto) (const std::vector&lt; char &gt; & compressed\_data, const std::vector&lt; [**BlockInfo**](structstar_1_1BlockInfo.md) &gt; & blocks, CompressionAlgorithm algorithm, char \* dst, size\_t dst\_capacity, [**ThreadPool**](classstar_1_1ThreadPool.md) \* thread\_pool=nullptr) <br>_One-pass sibling of decompressBlocks(): decompress every block straight into a caller-owned destination buffer instead of allocating and returning a std::vector._  |
|  size\_t | [**estimateCompressedSize**](#function-estimatecompressedsize) (const char \* data, size\_t data\_size, CompressionAlgorithm algorithm, size\_t block\_size) <br>_Estimate compressed size using deflateBound without actually compressing._  |
|  std::string | [**getLibraryVersion**](#function-getlibraryversion) () <br>_Get library version string._  |
|  uint64\_t | [**getNetworkRequestCount**](#function-getnetworkrequestcount) () <br> |
|  size\_t | [**getNumThreads**](#function-getnumthreads) () <br>_Get current thread count setting._  |
|  std::string | [**getS3Region**](#function-gets3region) () <br>_Get S3 region from environment or default._  |
|  uint64\_t | [**hash\_key**](#function-hash_key) (const std::string & key) <br>_Hash function for keys in global key registry._  |
|  [**FilePathInfo**](structstar_1_1FilePathInfo.md) | [**parseFilePath**](#function-parsefilepath) (const std::string & filename) <br>_Parse a file path and determine its type (local, HTTP, or S3)._  |
|  FileMode | [**parseModeString**](#function-parsemodestring) (const std::string & mode\_str) <br> |
|  UInt | [**read\_le**](#function-read_le) (std::istream & is) <br> |
|  uint16\_t | [**read\_u16**](#function-read_u16) (std::istream & is) <br> |
|  uint32\_t | [**read\_u32**](#function-read_u32) (std::istream & is) <br> |
|  uint64\_t | [**read\_u64**](#function-read_u64) (std::istream & is) <br> |
|  uint8\_t | [**read\_u8**](#function-read_u8) (std::istream & is) <br> |
|  void | [**resetNetworkRequestCount**](#function-resetnetworkrequestcount) () <br> |
|  void | [**setMinBlocksForThreading**](#function-setminblocksforthreading) (size\_t min\_blocks) <br>_Set minimum blocks threshold for using threading._  |
|  void | [**setMinBytesForThreading**](#function-setminbytesforthreading) (size\_t min\_bytes) <br>_Set minimum data size threshold for using threading._  |
|  void | [**setNumThreads**](#function-setnumthreads) (size\_t num\_threads) <br>_Set number of threads for parallel operations (all datasets)._  |
|  [**Slice**](structstar_1_1Slice.md) | [**slice\_all**](#function-slice_all) (size\_t dim\_size) <br> |
|  [**Slice**](structstar_1_1Slice.md) | [**slice\_range**](#function-slice_range) (size\_t start, size\_t stop) <br> |
|  CURLcode | [**star\_curl\_perform**](#function-star_curl_perform) (CURL \* handle) <br> |
|  bool | [**uses\_block\_shuffle**](#function-uses_block_shuffle) (CompressionAlgorithm c) <br>_Whether the shuffle prefilter is applied PER BLOCK (self-contained blocks). Such arrays are sliceable._  |
|  bool | [**uses\_global\_shuffle**](#function-uses_global_shuffle) (CompressionAlgorithm c) <br>_Whether the shuffle prefilter is applied across the WHOLE array (legacy layout). Such arrays are not sliceable._  |
|  bool | [**uses\_shuffle**](#function-uses_shuffle) (CompressionAlgorithm c) <br>_Whether a compression algorithm uses the byte-shuffle prefilter (either the legacy global variant or the per-block variant)._  |
|  void | [**write\_le**](#function-write_le) (std::ostream & os, UInt value) <br> |
|  void | [**write\_u16**](#function-write_u16) (std::ostream & os, uint16\_t v) <br> |
|  void | [**write\_u32**](#function-write_u32) (std::ostream & os, uint32\_t v) <br> |
|  void | [**write\_u64**](#function-write_u64) (std::ostream & os, uint64\_t v) <br> |
|  void | [**write\_u8**](#function-write_u8) (std::ostream & os, uint8\_t v) <br> |


## Public Static Functions

| Type | Name |
| ---: | :--- |
|  DataType | [**extract\_dtype\_from\_variant**](#function-extract_dtype_from_variant) (const ValueVariant & var) <br> |
|  std::vector&lt; size\_t &gt; | [**extract\_shape\_from\_variant**](#function-extract_shape_from_variant) (const ValueVariant & var) <br> |


























## Public Types Documentation




### enum CompressionAlgorithm 

```C++
enum star::CompressionAlgorithm {
    NONE = 0,
    GZIP = 1,
    ZSTD = 2,
    LZ4 = 3,
    GZIP_SHUFFLE = 4,
    LZ4_SHUFFLE = 5,
    GZIP_SHUFFLE_BLOCK = 6,
    LZ4_SHUFFLE_BLOCK = 7
};
```




<hr>



### enum DataType 

```C++
enum star::DataType {
    INT8 = 0,
    INT16 = 1,
    INT32 = 2,
    INT64 = 3,
    UINT8 = 4,
    UINT16 = 5,
    UINT32 = 6,
    UINT64 = 7,
    FLOAT32 = 8,
    FLOAT64 = 9,
    STRING = 10
};
```




<hr>



### enum FileMode 

```C++
enum star::FileMode {
    READ_WRITE,
    READ_ONLY
};
```




<hr>



### enum StorageClass 

_Storage classification for values (DEPRECATED - use StorageLocation)._ 
```C++
enum star::StorageClass {
    METADATA_BLOCK,
    SEPARATE_ARRAY,
    FORCE_SEPARATE
};
```




<hr>



### enum StorageLocation 

_Storage location state for unified storage model._ 
```C++
enum star::StorageLocation {
    PENDING,
    PERSISTED,
    CACHED
};
```




<hr>



### typedef ValueVariant 

```C++
using star::ValueVariant =  std::variant<
    NDArray<int8_t>, NDArray<int16_t>, NDArray<int32_t>, NDArray<int64_t>,
    NDArray<uint8_t>, NDArray<uint16_t>, NDArray<uint32_t>, NDArray<uint64_t>,
    NDArray<float>, NDArray<double>,
    NDArray<std::string>
>;
```




<hr>
## Public Attributes Documentation




### variable MAGIC\_STRING 

```C++
const char* star::MAGIC_STRING;
```




<hr>



### variable MAGIC\_STRING\_LENGTH 

```C++
const size_t star::MAGIC_STRING_LENGTH;
```




<hr>



### variable PROJECT\_NAME 

```C++
const std::string star::PROJECT_NAME;
```




<hr>



### variable g\_min\_blocks\_for\_threading 

```C++
size_t star::g_min_blocks_for_threading;
```




<hr>



### variable g\_min\_bytes\_for\_threading 

```C++
size_t star::g_min_bytes_for_threading;
```




<hr>



### variable g\_network\_request\_count 

```C++
std::atomic<uint64_t> star::g_network_request_count;
```




<hr>



### variable g\_num\_threads 

```C++
size_t star::g_num_threads;
```




<hr>
## Public Functions Documentation




### function base\_compression 

_The underlying block codec for a (possibly shuffle-prefiltered) algorithm._ 
```C++
inline CompressionAlgorithm star::base_compression (
    CompressionAlgorithm c
) 
```



The block (de)compressor only understands the base codecs; the shuffle variants differ only in a byte-reordering prefilter applied around it. 


        

<hr>



### function byte\_shuffle 

_Byte-shuffle: reorder_ `count` _elements of_`elem_size` _bytes so that byte plane 0 of every element comes first, then plane 1, etc._
```C++
inline void star::byte_shuffle (
    const char * in,
    char * out,
    size_t count,
    size_t elem_size
) 
```



Splits an array-of-structs byte layout into a struct-of-byte-planes layout. A no-op for elem\_size &lt;= 1. `out` must hold `count * elem_size` bytes. SIMD-accelerated on NEON for elem\_size {2,4,8}; scalar everywhere else (the scalar tail below is bit-identical to the historical loop). 


        

<hr>



### function byte\_shuffle\_blocked 

_Per-block byte-shuffle: apply byte\_shuffle() independently within each_ `block_size` _-byte chunk of a_`data_size` _-byte buffer._
```C++
inline void star::byte_shuffle_blocked (
    const char * in,
    char * out,
    size_t data_size,
    size_t elem_size,
    size_t block_size
) 
```



This is the prefilter for the BLOCK-shuffle codecs. Because the buffer is later cut into blocks at the SAME `block_size` boundaries by compressBlocksBuffered(), each compression block ends up holding exactly one self-contained shuffled chunk, so it can be un-shuffled on its own (enabling slicing) — unlike the global variant, whose byte planes span the whole array.


Each chunk is shuffled over its whole-element prefix (chunk\_size / elem\_size elements); any trailing bytes that don't form a complete element (only possible when block\_size is not a multiple of elem\_size) are copied through verbatim, and byte\_unshuffle\_blocked() reverses this exactly. `out` must hold `data_size` bytes. 


        

<hr>



### function byte\_unshuffle 

_Inverse of byte\_shuffle(): reassemble byte planes back into elements._ 
```C++
inline void star::byte_unshuffle (
    const char * in,
    char * out,
    size_t count,
    size_t elem_size
) 
```



This is the read hot path. SIMD-accelerated for elem\_size {2,4,8} on NEON (arm64) and SSE2 (x86-64); scalar everywhere else. The scalar tail is bit-identical to the historical loop, so decoded bytes never change. 


        

<hr>



### function byte\_unshuffle\_blocked 

_Inverse of byte\_shuffle\_blocked(): un-shuffle each_ `block_size` _chunk._
```C++
inline void star::byte_unshuffle_blocked (
    const char * in,
    char * out,
    size_t data_size,
    size_t elem_size,
    size_t block_size
) 
```




<hr>



### function compressBlocks 

_Compresses data in blocks and returns block metadata (legacy version)._ 
```C++
inline std::pair< std::vector< char >, std::vector< BlockInfo > > star::compressBlocks (
    const char * data,
    size_t data_size,
    CompressionAlgorithm algorithm,
    size_t block_size
) 
```





**Parameters:**


* `data` Raw uncompressed data 
* `data_size` Size of raw data 
* `algorithm` Compression algorithm to use 
* `block_size` Size of each uncompressed block 



**Returns:**

Pair of compressed data and block metadata 





        

<hr>



### function compressBlocksBuffered 

_Compresses data in blocks using pre-allocated buffer._ 
```C++
inline std::vector< BlockInfo > star::compressBlocksBuffered (
    const char * data,
    size_t data_size,
    CompressionAlgorithm algorithm,
    size_t block_size,
    std::vector< char > & compressed_output,
    std::vector< char > & temp_buffer,
    ThreadPool * thread_pool=nullptr
) 
```





**Parameters:**


* `data` Raw uncompressed data 
* `data_size` Size of raw data 
* `algorithm` Compression algorithm to use 
* `block_size` Size of each uncompressed block 
* `compressed_output` Pre-allocated output buffer (will be cleared and reused) 
* `temp_buffer` Pre-allocated temporary buffer for compression (will be resized as needed) 
* `thread_pool` Optional thread pool for parallel compression (nullptr = single-threaded) 



**Returns:**

Block metadata 





        

<hr>



### function datatype\_size 

_Get element size in bytes for a DataType._ 
```C++
inline size_t star::datatype_size (
    DataType dtype
) 
```





**Parameters:**


* `dtype` DataType to query 



**Returns:**

Size in bytes (0 for variable-length types like STRING) 





        

<hr>



### function datatype\_to\_string 

_Get string representation of DataType._ 
```C++
inline const char * star::datatype_to_string (
    DataType dtype
) 
```





**Parameters:**


* `dtype` DataType to convert 



**Returns:**

String representation 





        

<hr>



### function decompressBlocks 

_Decompresses specific blocks from compressed data._ 
```C++
inline std::vector< char > star::decompressBlocks (
    const std::vector< char > & compressed_data,
    const std::vector< BlockInfo > & blocks,
    CompressionAlgorithm algorithm,
    const std::vector< size_t > & block_indices={},
    ThreadPool * thread_pool=nullptr
) 
```





**Parameters:**


* `compressed_data` Full compressed data 
* `blocks` Block metadata 
* `algorithm` Compression algorithm used 
* `block_indices` Which blocks to decompress (empty = all) 
* `thread_pool` Optional thread pool for parallel decompression (nullptr = single-threaded) 



**Returns:**

Decompressed data 





        

<hr>



### function decompressBlocksInto 

_One-pass sibling of decompressBlocks(): decompress every block straight into a caller-owned destination buffer instead of allocating and returning a std::vector._ 
```C++
inline void star::decompressBlocksInto (
    const std::vector< char > & compressed_data,
    const std::vector< BlockInfo > & blocks,
    CompressionAlgorithm algorithm,
    char * dst,
    size_t dst_capacity,
    ThreadPool * thread_pool=nullptr
) 
```



decompressBlocks() allocates a full-size buffer, decodes into it, and returns it — the caller then copies that buffer into its final home (e.g. an [**NDArray**](classstar_1_1NDArray.md)), so every byte is written twice and a whole extra buffer is allocated. When the on-disk bytes ARE the final element bytes (fixed-width numeric arrays with no byte-shuffle prefilter to undo), that intermediate buffer is pure overhead: this function decodes each block directly to `dst + <block offset>`, giving a single pass over the data with no scratch allocation.


The per-codec decode calls are identical to decompressBlocks() (same LZ4/zlib entry points, same block layout, same threading gate) so the produced bytes are bit-for-bit the same; only the destination differs. It intentionally does NOT support a block-index subset (the fused path is always a whole-array read) — the slicing paths keep using decompressBlocks(). Left as a separate function rather than refactoring decompressBlocks() to delegate, so the existing return-a-vector path is untouched.




**Parameters:**


* `dst` Destination buffer; must hold at least the sum of block uncompressed sizes. 
* `dst_capacity` Size of `dst` in bytes (checked; a larger declared shape leaves the trailing bytes untouched, matching the partial memcpy the general path performs). 




        

<hr>



### function estimateCompressedSize 

_Estimate compressed size using deflateBound without actually compressing._ 
```C++
inline size_t star::estimateCompressedSize (
    const char * data,
    size_t data_size,
    CompressionAlgorithm algorithm,
    size_t block_size
) 
```



This provides an upper bound on the compressed size, which is useful for pre-calculating file positions before actual compression. The estimate is typically 10-30% larger than actual compressed size.




**Parameters:**


* `data` Raw uncompressed data 
* `data_size` Size of raw data 
* `algorithm` Compression algorithm to use 
* `block_size` Size of each uncompressed block 



**Returns:**

Estimated upper bound on compressed size 





        

<hr>



### function getLibraryVersion 

_Get library version string._ 
```C++
inline std::string star::getLibraryVersion () 
```





**Returns:**

Version string (e.g., "1.0.0") 





        

<hr>



### function getNetworkRequestCount 

```C++
inline uint64_t star::getNetworkRequestCount () 
```




<hr>



### function getNumThreads 

_Get current thread count setting._ 
```C++
inline size_t star::getNumThreads () 
```




<hr>



### function getS3Region 

_Get S3 region from environment or default._ 
```C++
inline std::string star::getS3Region () 
```




<hr>



### function hash\_key 

_Hash function for keys in global key registry._ 
```C++
inline uint64_t star::hash_key (
    const std::string & key
) 
```




<hr>



### function parseFilePath 

_Parse a file path and determine its type (local, HTTP, or S3)._ 
```C++
inline FilePathInfo star::parseFilePath (
    const std::string & filename
) 
```



Recognizes both the GDAL virtual-filesystem prefixes and the equivalent plain URL/URI forms, so either works interchangeably:
* S3: "/vsis3/bucket/key" OR "s3://bucket/key"
* HTTP: "/vsicurl/https://host/path" OR "https://host/path" / "http://..."
* anything else -&gt; a local filesystem path. The `/vsi*` prefixes stay supported for GDAL compatibility; the plain forms are the natural way to name a remote object and map to the same handling.




Defined unconditionally: [**StarDataset::open()**](classstar_1_1StarDataset.md#function-open-12)/ctor call it for every path, and the LOCAL branch (the common case) has no S3/curl dependency. Only the S3 region-resolution needs [**AWSConfigParser**](classstar_1_1AWSConfigParser.md), so that part is gated on ENABLE\_S3; without S3 support an S3 path is rejected with a clear error. 


        

<hr>



### function parseModeString 

```C++
inline FileMode star::parseModeString (
    const std::string & mode_str
) 
```




<hr>



### function read\_le 

```C++
template<typename UInt>
inline UInt star::read_le (
    std::istream & is
) 
```




<hr>



### function read\_u16 

```C++
inline uint16_t star::read_u16 (
    std::istream & is
) 
```




<hr>



### function read\_u32 

```C++
inline uint32_t star::read_u32 (
    std::istream & is
) 
```




<hr>



### function read\_u64 

```C++
inline uint64_t star::read_u64 (
    std::istream & is
) 
```




<hr>



### function read\_u8 

```C++
inline uint8_t star::read_u8 (
    std::istream & is
) 
```




<hr>



### function resetNetworkRequestCount 

```C++
inline void star::resetNetworkRequestCount () 
```




<hr>



### function setMinBlocksForThreading 

_Set minimum blocks threshold for using threading._ 
```C++
inline void star::setMinBlocksForThreading (
    size_t min_blocks
) 
```





**Parameters:**


* `min_blocks` Minimum number of blocks (default: 4) 




        

<hr>



### function setMinBytesForThreading 

_Set minimum data size threshold for using threading._ 
```C++
inline void star::setMinBytesForThreading (
    size_t min_bytes
) 
```





**Parameters:**


* `min_bytes` Minimum data size in bytes (default: 256KB) 




        

<hr>



### function setNumThreads 

_Set number of threads for parallel operations (all datasets)._ 
```C++
inline void star::setNumThreads (
    size_t num_threads
) 
```





**Parameters:**


* `num_threads` Number of threads (0 = auto-detect, 1 = single-threaded) 




        

<hr>



### function slice\_all 

```C++
inline Slice star::slice_all (
    size_t dim_size
) 
```




<hr>



### function slice\_range 

```C++
inline Slice star::slice_range (
    size_t start,
    size_t stop
) 
```




<hr>



### function star\_curl\_perform 

```C++
inline CURLcode star::star_curl_perform (
    CURL * handle
) 
```




<hr>



### function uses\_block\_shuffle 

_Whether the shuffle prefilter is applied PER BLOCK (self-contained blocks). Such arrays are sliceable._ 
```C++
inline bool star::uses_block_shuffle (
    CompressionAlgorithm c
) 
```




<hr>



### function uses\_global\_shuffle 

_Whether the shuffle prefilter is applied across the WHOLE array (legacy layout). Such arrays are not sliceable._ 
```C++
inline bool star::uses_global_shuffle (
    CompressionAlgorithm c
) 
```




<hr>



### function uses\_shuffle 

_Whether a compression algorithm uses the byte-shuffle prefilter (either the legacy global variant or the per-block variant)._ 
```C++
inline bool star::uses_shuffle (
    CompressionAlgorithm c
) 
```




<hr>



### function write\_le 

```C++
template<typename UInt>
inline void star::write_le (
    std::ostream & os,
    UInt value
) 
```




<hr>



### function write\_u16 

```C++
inline void star::write_u16 (
    std::ostream & os,
    uint16_t v
) 
```




<hr>



### function write\_u32 

```C++
inline void star::write_u32 (
    std::ostream & os,
    uint32_t v
) 
```




<hr>



### function write\_u64 

```C++
inline void star::write_u64 (
    std::ostream & os,
    uint64_t v
) 
```




<hr>



### function write\_u8 

```C++
inline void star::write_u8 (
    std::ostream & os,
    uint8_t v
) 
```




<hr>
## Public Static Functions Documentation




### function extract\_dtype\_from\_variant 

```C++
static inline DataType star::extract_dtype_from_variant (
    const ValueVariant & var
) 
```




<hr>



### function extract\_shape\_from\_variant 

```C++
static inline std::vector< size_t > star::extract_shape_from_variant (
    const ValueVariant & var
) 
```




<hr>

------------------------------
The documentation for this class was generated from the following file `StarDS/include/stards.h`




# Class star::StarDataset



[**ClassList**](annotated.md) **>** [**star**](namespacestar.md) **>** [**StarDataset**](classstar_1_1StarDataset.md)



_A cloud-optimized binary key-value store for serializable data types._ [More...](#detailed-description)

* `#include <stards.h>`



Inherits the following classes: std::enable_shared_from_this< StarDataset >














## Public Types

| Type | Name |
| ---: | :--- |
| typedef std::vector&lt; std::string &gt;::const\_iterator | [**const\_iterator**](#typedef-const_iterator)  <br> |
| typedef std::vector&lt; std::string &gt;::iterator | [**iterator**](#typedef-iterator)  <br> |




## Public Attributes

| Type | Name |
| ---: | :--- |
|  std::vector&lt; char &gt; \* | [**m\_capture\_image**](#variable-m_capture_image)   = `nullptr`<br> |
|  [**ColdStorage**](structstar_1_1ColdStorage.md) | [**m\_cold**](#variable-m_cold)  <br> |
|  std::vector&lt; char &gt; | [**m\_compress\_buffer**](#variable-m_compress_buffer)  <br> |
|  [**StarConfig**](structstar_1_1StarConfig.md) | [**m\_config**](#variable-m_config)  <br> |
|  std::vector&lt; ValueVariant &gt; | [**m\_data\_storage**](#variable-m_data_storage)  <br> |
|  [**FileHeader**](structstar_1_1FileHeader.md) | [**m\_file\_header**](#variable-m_file_header)  <br> |
|  FileMode | [**m\_file\_mode**](#variable-m_file_mode)  <br> |
|  std::string | [**m\_filename**](#variable-m_filename)  <br> |
|  bool | [**m\_flushed**](#variable-m_flushed)   = `false`<br> |
|  bool | [**m\_header\_dirty**](#variable-m_header_dirty)   = `false`<br> |
|  size\_t | [**m\_header\_size**](#variable-m_header_size)   = `0`<br> |
|  [**HotStorage**](structstar_1_1HotStorage.md) | [**m\_hot**](#variable-m_hot)  <br> |
|  [**KeyRegistry**](structstar_1_1KeyRegistry.md) | [**m\_key\_registry**](#variable-m_key_registry)  <br> |
|  std::unordered\_map&lt; std::string, size\_t &gt; | [**m\_key\_to\_index**](#variable-m_key_to_index)  <br> |
|  std::vector&lt; std::unordered\_map&lt; uint16\_t, size\_t &gt; &gt; | [**m\_layer\_metadata\_indices**](#variable-m_layer_metadata_indices)  <br> |
|  std::vector&lt; bool &gt; | [**m\_layer\_metadata\_loaded**](#variable-m_layer_metadata_loaded)  <br> |
|  [**LayerMetadataRegistry**](structstar_1_1LayerMetadataRegistry.md) | [**m\_layer\_metadata\_registry**](#variable-m_layer_metadata_registry)  <br> |
|  std::map&lt; std::string, std::vector&lt; uint64\_t &gt; &gt; | [**m\_layer\_presence**](#variable-m_layer_presence)  <br> |
|  std::vector&lt; char &gt; | [**m\_memory\_source**](#variable-m_memory_source)  <br> |
|  std::unordered\_map&lt; size\_t, DataType &gt; | [**m\_metadata\_dtypes**](#variable-m_metadata_dtypes)  <br> |
|  bool | [**m\_metadata\_loaded**](#variable-m_metadata_loaded)   = `false`<br> |
|  std::unordered\_map&lt; size\_t, std::vector&lt; size\_t &gt; &gt; | [**m\_metadata\_shapes**](#variable-m_metadata_shapes)  <br> |
|  std::shared\_mutex | [**m\_mutex**](#variable-m_mutex)  <br> |
|  [**OpenOptions**](structstar_1_1OpenOptions.md) | [**m\_open\_options**](#variable-m_open_options)  <br> |
|  [**FilePathInfo**](structstar_1_1FilePathInfo.md) | [**m\_path\_info**](#variable-m_path_info)  <br> |
|  std::unique\_ptr&lt; [**RangeReader**](classstar_1_1RangeReader.md) &gt; | [**m\_reader**](#variable-m_reader)  <br> |
|  std::unique\_ptr&lt; [**S3Credentials**](structstar_1_1S3Credentials.md) &gt; | [**m\_s3\_credentials**](#variable-m_s3_credentials)  <br> |
|  std::vector&lt; char &gt; | [**m\_serialize\_buffer**](#variable-m_serialize_buffer)  <br> |
|  std::unique\_ptr&lt; [**ThreadPool**](classstar_1_1ThreadPool.md) &gt; | [**m\_thread\_pool**](#variable-m_thread_pool)  <br> |
|  [**MetadataAccessor**](classstar_1_1MetadataAccessor.md) | [**meta**](#variable-meta)  <br> |
















## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**StarDataset**](#function-stardataset-13) (const std::string & fname, FileMode mode, const [**StarConfig**](structstar_1_1StarConfig.md) \* config, const [**OpenOptions**](structstar_1_1OpenOptions.md) & open\_options={}, std::vector&lt; char &gt; \* memory\_source=nullptr) <br>_Constructor with compression options._  |
|   | [**StarDataset**](#function-stardataset-23) (const StarDataset &) = delete<br> |
|   | [**StarDataset**](#function-stardataset-33) (StarDataset &&) = delete<br> |
|  size\_t | [**array\_length**](#function-array_length) (const std::string & key) const<br>_Length of a stored array's first (outermost) dimension._  |
|  iterator | [**begin**](#function-begin-12) () <br>_Returns an iterator to the beginning of the index._  |
|  const\_iterator | [**begin**](#function-begin-22) () const<br>_Returns a const iterator to the beginning of the index._  |
|  size\_t | [**calculateHeaderSize**](#function-calculateheadersize) () <br>_Calculates the size of the header based on current index._  |
|  const\_iterator | [**cbegin**](#function-cbegin) () const<br>_Returns a const iterator to the beginning of the index._  |
|  const\_iterator | [**cend**](#function-cend) () const<br>_Returns a const iterator to the end of the index._  |
|  void | [**close**](#function-close) () <br>_Flush all pending writes to disk._  |
|  std::vector&lt; char &gt; | [**compress\_single\_block**](#function-compress_single_block) (const std::vector&lt; char &gt; & uncompressed, CompressionAlgorithm compression) <br>_Compress a single block (helper for layer metadata)._  |
|  bool | [**contains**](#function-contains) (const std::string & key) const<br>_Check whether a key exists in the dataset._  |
|  std::shared\_ptr&lt; [**LayerView**](classstar_1_1LayerView.md) &gt; | [**create\_layer**](#function-create_layer) (const std::string & layer\_name) <br>_Create new layer and return view._  |
|  ValueVariant | [**decode\_array\_bytes**](#function-decode_array_bytes) (const std::vector&lt; char &gt; & compressed\_data, const std::vector&lt; [**BlockInfo**](structstar_1_1BlockInfo.md) &gt; & blocks, CompressionAlgorithm compression, DataType dtype, const std::vector&lt; size\_t &gt; & shape, [**ThreadPool**](classstar_1_1ThreadPool.md) \* thread\_pool) <br> |
|  ValueVariant | [**decode\_numeric\_blocks\_into\_ndarray**](#function-decode_numeric_blocks_into_ndarray) (const std::vector&lt; char &gt; & compressed\_data, const std::vector&lt; [**BlockInfo**](structstar_1_1BlockInfo.md) &gt; & blocks, CompressionAlgorithm compression, DataType dtype, const std::vector&lt; size\_t &gt; & shape, [**ThreadPool**](classstar_1_1ThreadPool.md) \* thread\_pool) <br>_Decode one array entry's compressed bytes into a ValueVariant._  |
|  std::vector&lt; char &gt; | [**decompress\_single\_block**](#function-decompress_single_block) (const std::vector&lt; char &gt; & compressed, CompressionAlgorithm compression) <br>_Decompress a single block (helper for layer metadata)._  |
|  std::string | [**deserializeKey**](#function-deserializekey) (std::istream & is) <br>_Deserializes a key from the input stream._  |
|  ValueVariant | [**deserialize\_typed\_value**](#function-deserialize_typed_value) (std::istream & is, DataType dtype, const std::vector&lt; size\_t &gt; & shape, size\_t data\_len) <br>_Deserializes typed value from stream._  |
|  ValueVariant | [**deserialize\_typed\_value\_bytes**](#function-deserialize_typed_value_bytes) (const char \* data, size\_t data\_len, DataType dtype, const std::vector&lt; size\_t &gt; & shape, const [**ByteUnshuffleSpec**](structstar_1_1ByteUnshuffleSpec.md) & unshuffle={}) <br>_Zero-stream variant of_ [_**deserialize\_typed\_value()**_](classstar_1_1StarDataset.md#function-deserialize_typed_value) _for the array load hot path: deserialize directly from a contiguous byte buffer._ |
|  DataType | [**dtype\_of**](#function-dtype_of) (const std::string & key) const<br>_Get the element data type of a stored array._  |
|  iterator | [**end**](#function-end-12) () <br>_Returns an iterator to the end of the index._  |
|  const\_iterator | [**end**](#function-end-22) () const<br>_Returns a const iterator to the end of the index._  |
|  void | [**ensure\_layer\_metadata\_loaded**](#function-ensure_layer_metadata_loaded) (size\_t layer\_idx) <br>_Loads metadata block from file if not already loaded (supports HTTP/remote URLs)._  |
|  void | [**flush**](#function-flush) () <br>_Writes all data to the file (including metadata block)._  |
|  void | [**flush\_quiet**](#function-flush_quiet) () <br>_Flush without throwing on read-only (silently skips instead)._  |
|  [**NDArray**](classstar_1_1NDArray.md)&lt; T &gt; | [**get**](#function-get) (const std::string & key) <br>_Get array from storage._  |
|  std::vector&lt; std::string &gt; | [**get\_all\_keys**](#function-get_all_keys) () <br>_Gets all keys (metadata block + separate arrays)._  |
|  const [**FileHeader**](structstar_1_1FileHeader.md) & | [**get\_file\_header**](#function-get_file_header) () const<br>_Get file header with version information._  |
|  std::string | [**get\_filename**](#function-get_filename) () const<br>_Get current filename._  |
|  std::shared\_ptr&lt; [**LayerView**](classstar_1_1LayerView.md) &gt; | [**get\_layer**](#function-get_layer) (const std::string & layer\_name) <br>_Get existing layer view._  |
|  size\_t | [**get\_metadata\_count**](#function-get_metadata_count) () const<br>_Gets count of entries in metadata block._  |
|  std::vector&lt; std::string &gt; | [**get\_metadata\_keys**](#function-get_metadata_keys) () const<br>_Gets list of keys in metadata block._  |
|  [**NDArray**](classstar_1_1NDArray.md)&lt; T &gt; | [**get\_slice**](#function-get_slice) (const std::string & key, const std::vector&lt; [**Slice**](structstar_1_1Slice.md) &gt; & slices) <br>_Gets n-dimensional slice of an array._  |
|  bool | [**has\_layer**](#function-has_layer) (const std::string & layer\_name) const<br>_Check if layer exists._  |
|  bool | [**has\_metadata\_block**](#function-has_metadata_block) () const<br>_Checks if file has metadata block._  |
|  bool | [**is\_metadata\_loaded**](#function-is_metadata_loaded) () const<br>_Checks if metadata block has been loaded._  |
|  bool | [**is\_read\_only**](#function-is_read_only) () const<br>_Check if file is in read-only mode._  |
|  bool | [**is\_sliceable**](#function-is_sliceable) (const std::string & key) const<br>_Checks if an array supports slicing._  |
|  bool | [**key\_in\_layer**](#function-key_in_layer) (const std::string & key, const std::string & layer\_name) const<br>_Check if key exists in specific layer using bit-mask (O(1))._  |
|  bool | [**layer\_inheritance**](#function-layer_inheritance) () const<br> |
|  std::vector&lt; std::string &gt; | [**list\_layers**](#function-list_layers) () const<br>_Get list of all layer names._  |
|  void | [**loadIndex**](#function-loadindex) () <br>_Loads the index from the file._  |
|  void | [**load\_all\_metadata**](#function-load_all_metadata) () <br>_Load all layer metadata blocks at once (v3 format)._  |
|  void | [**load\_entry**](#function-load_entry) (size\_t idx) <br>_Load entry from disk into memory._  |
|  void | [**load\_layer\_metadata**](#function-load_layer_metadata) (const std::string & layer\_name) <br>_Load a specific layer's metadata block (v3 format)._  |
|  void | [**load\_metadata\_block**](#function-load_metadata_block) () <br> |
|  const [**OpenOptions**](structstar_1_1OpenOptions.md) & | [**open\_options**](#function-open_options) () const<br>_Read-time options (e.g. layer inheritance) for this dataset._  |
|  [**StarDataset**](classstar_1_1StarDataset.md#function-stardataset-13) & | [**operator=**](#function-operator) (const [**StarDataset**](classstar_1_1StarDataset.md#function-stardataset-13) &) = delete<br> |
|  [**StarDataset**](classstar_1_1StarDataset.md#function-stardataset-13) & | [**operator=**](#function-operator_1) ([**StarDataset**](classstar_1_1StarDataset.md#function-stardataset-13) &&) = delete<br> |
|  void | [**parse\_layer\_metadata\_block**](#function-parse_layer_metadata_block) (size\_t layer\_idx, const std::vector&lt; char &gt; & decompressed) <br>_Parse a layer's metadata block (STARMeta format)._  |
|  void | [**prefetch**](#function-prefetch) (const std::vector&lt; std::string &gt; & keys) <br>_Load several arrays into cache in parallel (a read-ahead hint)._  |
|  void | [**print\_header**](#function-print_header) () const<br> |
|  void | [**put**](#function-put-12) (const std::string & key, [**NDArray**](classstar_1_1NDArray.md)&lt; T &gt; && value) <br>_Store array as separate compressed array (not in metadata block)._  |
|  void | [**put**](#function-put-22) (const std::string & key, const [**NDArray**](classstar_1_1NDArray.md)&lt; T &gt; & value) <br>_Store array as separate compressed array (const lvalue overload)._  |
|  std::vector&lt; char &gt; | [**read\_range**](#function-read_range) (size\_t position, size\_t len) <br>_Read exactly [position, position+len) from the backing file via the persistent reader. Returns the bytes actually read._  |
|  [**RangeReader**](classstar_1_1RangeReader.md) & | [**reader**](#function-reader) () <br>_Return the dataset's persistent byte-range reader, creating it on first use. One reader (= one reused connection for remote files) is shared by every read path — the index load, layer-metadata loads, and array-block reads — so remote reads never open a new connection or issue a HEAD per read._  |
|  void | [**save\_to**](#function-save_to) (const std::string & target\_path) <br> |
|  std::vector&lt; char &gt; | [**serialize\_array\_data**](#function-serialize_array_data-12) (const std::vector&lt; T &gt; & data) const<br>_Serialize an_ [_**NDArray**_](classstar_1_1NDArray.md) _'s element data into a byte buffer for block storage, in the SAME wire format_[_**deserialize\_typed\_value()**_](classstar_1_1StarDataset.md#function-deserialize_typed_value) _reads._ |
|  std::vector&lt; char &gt; | [**serialize\_array\_data**](#function-serialize_array_data-22) (const std::vector&lt; T &gt; & data, CompressionAlgorithm codec, size\_t block\_size) const<br>_Serialize array data, applying the byte-shuffle prefilter if the codec is a \*\_SHUFFLE variant._  |
|  void | [**serialize\_layer\_metadata\_block**](#function-serialize_layer_metadata_block) (std::ostream & os, const std::string & layer\_name) <br>_Serializes a single layer's metadata in STARMeta format (v1)._  |
|  void | [**serialize\_metadata\_block**](#function-serialize_metadata_block) (std::ostream & os) const<br>_Serializes metadata block to stream (OLD FORMAT - deprecated)._  |
|  std::enable\_if&lt; std::is\_same&lt; T, std::string &gt;::value, void &gt;::type | [**serialize\_metadata\_value**](#function-serialize_metadata_value-12) (std::ostream & os, const std::vector&lt; T &gt; & data) const<br>_Helper to serialize metadata value (string specialization)._  |
|  std::enable\_if&lt;!std::is\_same&lt; T, std::string &gt;::value, void &gt;::type | [**serialize\_metadata\_value**](#function-serialize_metadata_value-22) (std::ostream & os, const std::vector&lt; T &gt; & data) const<br>_Helper to serialize metadata value (numeric types)._  |
|  void | [**set\_layer\_inheritance**](#function-set_layer_inheritance) (bool on) <br> |
|  void | [**set\_layer\_presence**](#function-set_layer_presence) (const std::string & layer\_name, const std::string & key, bool present) <br>_Set layer presence for a data array key._  |
|  void | [**set\_open\_options**](#function-set_open_options) (const [**OpenOptions**](structstar_1_1OpenOptions.md) & opts) <br> |
|  std::vector&lt; size\_t &gt; | [**shape\_of**](#function-shape_of) (const std::string & key) const<br>_Full shape (all dimensions) of a stored data array._  |
|  size\_t | [**size**](#function-size) () const<br>_Removes a key-value pair from the store._  |
|  bool | [**useThreading**](#function-usethreading) (size\_t num\_blocks, size\_t data\_size) const<br>_Check if threading should be used based on workload._  |
|  std::vector&lt; char &gt; | [**write\_bytes**](#function-write_bytes) () <br>_Serialize the dataset to an in-memory byte buffer._  |
|   | [**~StarDataset**](#function-stardataset) () <br>_Destructor - writes all pending changes to disk (RAII)._  |


## Public Static Functions

| Type | Name |
| ---: | :--- |
|  std::shared\_ptr&lt; [**StarDataset**](classstar_1_1StarDataset.md#function-stardataset-13) &gt; | [**create**](#function-create) (const std::string & filename, const [**StarConfig**](structstar_1_1StarConfig.md) & config=[**StarConfig**](structstar_1_1StarConfig.md)()) <br>_Create a new Star dataset file._  |
|  std::shared\_ptr&lt; [**StarDataset**](classstar_1_1StarDataset.md#function-stardataset-13) &gt; | [**open**](#function-open-12) (const std::string & filename, FileMode mode=FileMode::READ\_WRITE, const [**OpenOptions**](structstar_1_1OpenOptions.md) & opts={}) <br>_Open an existing Star dataset file._  |
|  std::shared\_ptr&lt; [**StarDataset**](classstar_1_1StarDataset.md#function-stardataset-13) &gt; | [**open**](#function-open-22) (const std::string & filename, const std::string & mode\_str, const [**OpenOptions**](structstar_1_1OpenOptions.md) & opts={}) <br>_Open an existing Star dataset file (string mode overload)._  |
|  std::shared\_ptr&lt; [**StarDataset**](classstar_1_1StarDataset.md#function-stardataset-13) &gt; | [**open\_bytes**](#function-open_bytes-12) (std::vector&lt; char &gt; bytes, const [**OpenOptions**](structstar_1_1OpenOptions.md) & opts={}) <br>_Open a dataset from an in-memory byte buffer._  |
|  std::shared\_ptr&lt; [**StarDataset**](classstar_1_1StarDataset.md#function-stardataset-13) &gt; | [**open\_bytes**](#function-open_bytes-22) (const void \* data, size\_t size, const [**OpenOptions**](structstar_1_1OpenOptions.md) & opts={}) <br>_Convenience overload taking a raw pointer + length (e.g. from a C buffer, Python bytes, or a mapped region)._  |


























## Detailed Description


This class implements a binary key-value store that can persist data to disk in a cloud-optimized format. It uses a single file with an index section followed by data for efficient cloud storage and retrieval. Large arrays are chunked for better performance over networks. 


    
## Public Types Documentation




### typedef const\_iterator 

```C++
using star::StarDataset::const_iterator = std::vector<std::string>::const_iterator;
```




<hr>



### typedef iterator 

```C++
using star::StarDataset::iterator = std::vector<std::string>::iterator;
```




<hr>
## Public Attributes Documentation




### variable m\_capture\_image 

```C++
std::vector<char>* star::StarDataset::m_capture_image;
```




<hr>



### variable m\_cold 

```C++
ColdStorage star::StarDataset::m_cold;
```




<hr>



### variable m\_compress\_buffer 

```C++
std::vector<char> star::StarDataset::m_compress_buffer;
```




<hr>



### variable m\_config 

```C++
StarConfig star::StarDataset::m_config;
```




<hr>



### variable m\_data\_storage 

```C++
std::vector<ValueVariant> star::StarDataset::m_data_storage;
```




<hr>



### variable m\_file\_header 

```C++
FileHeader star::StarDataset::m_file_header;
```




<hr>



### variable m\_file\_mode 

```C++
FileMode star::StarDataset::m_file_mode;
```




<hr>



### variable m\_filename 

```C++
std::string star::StarDataset::m_filename;
```




<hr>



### variable m\_flushed 

```C++
bool star::StarDataset::m_flushed;
```




<hr>



### variable m\_header\_dirty 

```C++
bool star::StarDataset::m_header_dirty;
```




<hr>



### variable m\_header\_size 

```C++
size_t star::StarDataset::m_header_size;
```




<hr>



### variable m\_hot 

```C++
HotStorage star::StarDataset::m_hot;
```




<hr>



### variable m\_key\_registry 

```C++
KeyRegistry star::StarDataset::m_key_registry;
```




<hr>



### variable m\_key\_to\_index 

```C++
std::unordered_map<std::string, size_t> star::StarDataset::m_key_to_index;
```




<hr>



### variable m\_layer\_metadata\_indices 

```C++
std::vector<std::unordered_map<uint16_t, size_t> > star::StarDataset::m_layer_metadata_indices;
```




<hr>



### variable m\_layer\_metadata\_loaded 

```C++
std::vector<bool> star::StarDataset::m_layer_metadata_loaded;
```




<hr>



### variable m\_layer\_metadata\_registry 

```C++
LayerMetadataRegistry star::StarDataset::m_layer_metadata_registry;
```




<hr>



### variable m\_layer\_presence 

```C++
std::map<std::string, std::vector<uint64_t> > star::StarDataset::m_layer_presence;
```




<hr>



### variable m\_memory\_source 

```C++
std::vector<char> star::StarDataset::m_memory_source;
```




<hr>



### variable m\_metadata\_dtypes 

```C++
std::unordered_map<size_t, DataType> star::StarDataset::m_metadata_dtypes;
```




<hr>



### variable m\_metadata\_loaded 

```C++
bool star::StarDataset::m_metadata_loaded;
```




<hr>



### variable m\_metadata\_shapes 

```C++
std::unordered_map<size_t, std::vector<size_t> > star::StarDataset::m_metadata_shapes;
```




<hr>



### variable m\_mutex 

```C++
std::shared_mutex star::StarDataset::m_mutex;
```




<hr>



### variable m\_open\_options 

```C++
OpenOptions star::StarDataset::m_open_options;
```




<hr>



### variable m\_path\_info 

```C++
FilePathInfo star::StarDataset::m_path_info;
```




<hr>



### variable m\_reader 

```C++
std::unique_ptr<RangeReader> star::StarDataset::m_reader;
```




<hr>



### variable m\_s3\_credentials 

```C++
std::unique_ptr<S3Credentials> star::StarDataset::m_s3_credentials;
```




<hr>



### variable m\_serialize\_buffer 

```C++
std::vector<char> star::StarDataset::m_serialize_buffer;
```




<hr>



### variable m\_thread\_pool 

```C++
std::unique_ptr<ThreadPool> star::StarDataset::m_thread_pool;
```




<hr>



### variable meta 

```C++
MetadataAccessor star::StarDataset::meta;
```




<hr>
## Public Functions Documentation




### function StarDataset [1/3]

_Constructor with compression options._ 
```C++
inline star::StarDataset::StarDataset (
    const std::string & fname,
    FileMode mode,
    const StarConfig * config,
    const OpenOptions & open_options={},
    std::vector< char > * memory_source=nullptr
) 
```





**Parameters:**


* `fname` Filename to use for storage 
* `mode` File open mode (READ\_WRITE or READ\_ONLY) 
* `config` Optional configuration (nullptr to load from file)

NOTE: Constructor is public to enable std::make\_shared, but you should use the static factory methods [**create()**](classstar_1_1StarDataset.md#function-create) and [**open()**](classstar_1_1StarDataset.md#function-open-12) instead. 


        

<hr>



### function StarDataset [2/3]

```C++
star::StarDataset::StarDataset (
    const StarDataset &
) = delete
```




<hr>



### function StarDataset [3/3]

```C++
star::StarDataset::StarDataset (
    StarDataset &&
) = delete
```




<hr>



### function array\_length 

_Length of a stored array's first (outermost) dimension._ 
```C++
inline size_t star::StarDataset::array_length (
    const std::string & key
) const
```



Read from the index that [**open()**](classstar_1_1StarDataset.md#function-open-12) loads up front (m\_cold.shapes) — NO block data is fetched, so this is cheap even for a multi-GB array or a remote (/vsicurl, /vsis3) source, and lets a caller size a streaming read (get\_slice windows) before pulling any values. For a 1-D column this is the element count; for N-D it is shape[0] (the row count). Scalars report 1.




**Parameters:**


* `key` Array key (must be a separately-stored data array, not metadata) 



**Returns:**

Length of dimension 0 




**Exception:**


* `std::runtime_error` if the key is not a stored data array 




        

<hr>



### function begin [1/2]

_Returns an iterator to the beginning of the index._ 
```C++
inline iterator star::StarDataset::begin () 
```





**Returns:**

Iterator to the first key-value pair 





        

<hr>



### function begin [2/2]

_Returns a const iterator to the beginning of the index._ 
```C++
inline const_iterator star::StarDataset::begin () const
```





**Returns:**

Const iterator to the first key-value pair 





        

<hr>



### function calculateHeaderSize 

_Calculates the size of the header based on current index._ 
```C++
inline size_t star::StarDataset::calculateHeaderSize () 
```





**Returns:**

Size of the header in bytes 





        

<hr>



### function cbegin 

_Returns a const iterator to the beginning of the index._ 
```C++
inline const_iterator star::StarDataset::cbegin () const
```





**Returns:**

Const iterator to the first key-value pair 





        

<hr>



### function cend 

_Returns a const iterator to the end of the index._ 
```C++
inline const_iterator star::StarDataset::cend () const
```





**Returns:**

Const iterator to one past the last key-value pair 





        

<hr>



### function close 

_Flush all pending writes to disk._ 
```C++
inline void star::StarDataset::close () 
```



This is automatically called on context manager exit or object destruction. You can call this manually to ensure data is persisted without closing the dataset. 


        

<hr>



### function compress\_single\_block 

_Compress a single block (helper for layer metadata)._ 
```C++
inline std::vector< char > star::StarDataset::compress_single_block (
    const std::vector< char > & uncompressed,
    CompressionAlgorithm compression
) 
```





**Parameters:**


* `uncompressed` Uncompressed data 
* `compression` Compression algorithm 



**Returns:**

Compressed data 





        

<hr>



### function contains 

_Check whether a key exists in the dataset._ 
```C++
inline bool star::StarDataset::contains (
    const std::string & key
) const
```



Returns true if `key` names either a stored array (array namespace) or a metadata-block value (metadata namespace). Layer-prefixed internal keys are matched as-is. Use this for membership tests; it does not load any data.




**Parameters:**


* `key` Key to look up 



**Returns:**

true if the key exists in either namespace 





        

<hr>



### function create\_layer 

_Create new layer and return view._ 
```C++
inline std::shared_ptr< LayerView > star::StarDataset::create_layer (
    const std::string & layer_name
) 
```





**Parameters:**


* `layer_name` Name of new layer 



**Returns:**

Shared pointer to [**LayerView**](classstar_1_1LayerView.md) 




**Exception:**


* `std::runtime_error` if layer already exists 




        

<hr>



### function decode\_array\_bytes 

```C++
inline ValueVariant star::StarDataset::decode_array_bytes (
    const std::vector< char > & compressed_data,
    const std::vector< BlockInfo > & blocks,
    CompressionAlgorithm compression,
    DataType dtype,
    const std::vector< size_t > & shape,
    ThreadPool * thread_pool
) 
```




<hr>



### function decode\_numeric\_blocks\_into\_ndarray 

_Decode one array entry's compressed bytes into a ValueVariant._ 
```C++
inline ValueVariant star::StarDataset::decode_numeric_blocks_into_ndarray (
    const std::vector< char > & compressed_data,
    const std::vector< BlockInfo > & blocks,
    CompressionAlgorithm compression,
    DataType dtype,
    const std::vector< size_t > & shape,
    ThreadPool * thread_pool
) 
```



This is the CPU half of an array load (everything after the ranged read): decompress the blocks, reverse any byte-shuffle prefilter, and materialize the [**NDArray**](classstar_1_1NDArray.md). It is a pure function of its arguments — it reads no shared mutable state and writes none — so it is safe to run concurrently on thread-pool workers, which is what [**prefetch()**](classstar_1_1StarDataset.md#function-prefetch) relies on to decode many columns in parallel.


`thread_pool` controls only INTRA-array block parallelism inside decompressBlocks(). A batch caller that is already parallelizing ACROSS arrays MUST pass nullptr here: a pool worker that enqueued to and waited on the same pool could deadlock. The single-array load path (load\_entry) passes the pool so a lone large array still decompresses its blocks in parallel.


One-pass decode for the fast path: build the [**NDArray&lt;T&gt;**](classstar_1_1NDArray.md) and decompress every block directly into its storage.


Only reached for fixed-width numeric arrays with no byte-shuffle prefilter, where the decompressed block bytes are exactly the element bytes. Produces the identical [**NDArray**](classstar_1_1NDArray.md) the general (decompressBlocks + memcpy) path would, without the intermediate full-size buffer or the second pass. Pure function of its arguments — safe to run on pool workers, same as decode\_array\_bytes(). 


        

<hr>



### function decompress\_single\_block 

_Decompress a single block (helper for layer metadata)._ 
```C++
inline std::vector< char > star::StarDataset::decompress_single_block (
    const std::vector< char > & compressed,
    CompressionAlgorithm compression
) 
```





**Parameters:**


* `compressed` Compressed data 
* `compression` Compression algorithm 



**Returns:**

Decompressed data 





        

<hr>



### function deserializeKey 

_Deserializes a key from the input stream._ 
```C++
inline std::string star::StarDataset::deserializeKey (
    std::istream & is
) 
```





**Parameters:**


* `is` Input stream 



**Returns:**

Deserialized key 





        

<hr>



### function deserialize\_typed\_value 

_Deserializes typed value from stream._ 
```C++
inline ValueVariant star::StarDataset::deserialize_typed_value (
    std::istream & is,
    DataType dtype,
    const std::vector< size_t > & shape,
    size_t data_len
) 
```





**Parameters:**


* `is` Input stream 
* `dtype` Data type 
* `shape` Array shape 
* `data_len` Data length in bytes 



**Returns:**

ValueVariant containing the deserialized value 





        

<hr>



### function deserialize\_typed\_value\_bytes 

_Zero-stream variant of_ [_**deserialize\_typed\_value()**_](classstar_1_1StarDataset.md#function-deserialize_typed_value) _for the array load hot path: deserialize directly from a contiguous byte buffer._
```C++
inline ValueVariant star::StarDataset::deserialize_typed_value_bytes (
    const char * data,
    size_t data_len,
    DataType dtype,
    const std::vector< size_t > & shape,
    const ByteUnshuffleSpec & unshuffle={}
) 
```



The stream version copies the buffer INTO a std::stringstream and then reads it back OUT into the [**NDArray**](classstar_1_1NDArray.md) — two extra full-buffer passes plus stream overhead. For fixed-width numeric arrays (the common case) the wire format is just raw native-endian bytes, so a single std::memcpy reproduces the exact same result an order of magnitude faster. Strings are variable-width (length-prefixed) and rare on this path, so they delegate to the stream version unchanged — behavior is identical for every dtype.


When `unshuffle.active`, the incoming bytes are byte-planes from a shuffle codec: instead of a plain memcpy we run byte\_unshuffle straight INTO the [**NDArray**](classstar_1_1NDArray.md) storage. This fuses the transpose with the fill, eliminating the scratch buffer and the extra full-buffer pass the caller would otherwise do (decompress → scratch → unshuffle → scratch2 → memcpy → [**NDArray**](classstar_1_1NDArray.md) becomes decompress → scratch → unshuffle → [**NDArray**](classstar_1_1NDArray.md)). The default (`active == false`) is a byte-identical plain memcpy, so non-shuffle codecs are unchanged. 


        

<hr>



### function dtype\_of 

_Get the element data type of a stored array._ 
```C++
inline DataType star::StarDataset::dtype_of (
    const std::string & key
) const
```



Looks up the array namespace (values stored via [**put()**](classstar_1_1StarDataset.md#function-put-12)/put&lt;T&gt;()). This lets callers dispatch on the concrete type without probing every [**get&lt;T&gt;()**](classstar_1_1StarDataset.md#function-get) overload. For metadata-namespace values use meta.get(key)-&gt;dtype instead.




**Parameters:**


* `key` Array key 



**Returns:**

DataType of the stored array 




**Exception:**


* `std::runtime_error` if the key is not an array (not found in array storage) 




        

<hr>



### function end [1/2]

_Returns an iterator to the end of the index._ 
```C++
inline iterator star::StarDataset::end () 
```





**Returns:**

Iterator to one past the last key-value pair 





        

<hr>



### function end [2/2]

_Returns a const iterator to the end of the index._ 
```C++
inline const_iterator star::StarDataset::end () const
```





**Returns:**

Iterator to one past the last key-value pair 





        

<hr>



### function ensure\_layer\_metadata\_loaded 

_Loads metadata block from file if not already loaded (supports HTTP/remote URLs)._ 
```C++
inline void star::StarDataset::ensure_layer_metadata_loaded (
    size_t layer_idx
) 
```



Ensure a specific layer's metadata is loaded (v3 format) 

**Parameters:**


* `layer_idx` Index of the layer to load 




        

<hr>



### function flush 

_Writes all data to the file (including metadata block)._ 
```C++
inline void star::StarDataset::flush () 
```




<hr>



### function flush\_quiet 

_Flush without throwing on read-only (silently skips instead)._ 
```C++
inline void star::StarDataset::flush_quiet () 
```



Used by [**close()**](classstar_1_1StarDataset.md#function-close) and the destructor, where flushing is best-effort cleanup rather than an explicit persist request — a read-only dataset must be destructible without raising. 


        

<hr>



### function get 

_Get array from storage._ 
```C++
template<typename T>
inline NDArray < T > star::StarDataset::get (
    const std::string & key
) 
```



Retrieves array from either metadata block or separate storage.




**Parameters:**


* `key` Array key 



**Returns:**

[**NDArray**](classstar_1_1NDArray.md) 





        

<hr>



### function get\_all\_keys 

_Gets all keys (metadata block + separate arrays)._ 
```C++
inline std::vector< std::string > star::StarDataset::get_all_keys () 
```





**Returns:**

Vector of all keys in the store 





        

<hr>



### function get\_file\_header 

_Get file header with version information._ 
```C++
inline const FileHeader & star::StarDataset::get_file_header () const
```





**Returns:**

Reference to [**FileHeader**](structstar_1_1FileHeader.md) 





        

<hr>



### function get\_filename 

_Get current filename._ 
```C++
inline std::string star::StarDataset::get_filename () const
```





**Returns:**

Filename 





        

<hr>



### function get\_layer 

_Get existing layer view._ 
```C++
inline std::shared_ptr< LayerView > star::StarDataset::get_layer (
    const std::string & layer_name
) 
```





**Parameters:**


* `layer_name` Name of the layer 



**Returns:**

Shared pointer to [**LayerView**](classstar_1_1LayerView.md) 




**Exception:**


* `std::runtime_error` if layer doesn't exist 




        

<hr>



### function get\_metadata\_count 

_Gets count of entries in metadata block._ 
```C++
inline size_t star::StarDataset::get_metadata_count () const
```





**Returns:**

Number of metadata entries (excludes layer-prefixed internal keys) 





        

<hr>



### function get\_metadata\_keys 

_Gets list of keys in metadata block._ 
```C++
inline std::vector< std::string > star::StarDataset::get_metadata_keys () const
```





**Returns:**

Vector of key names (excludes layer-prefixed internal keys) 





        

<hr>



### function get\_slice 

_Gets n-dimensional slice of an array._ 
```C++
template<typename T>
inline NDArray < T > star::StarDataset::get_slice (
    const std::string & key,
    const std::vector< Slice > & slices
) 
```



IMPORTANT: This function only works with arrays stored as separate compressed arrays with block structure. Arrays stored in the metadata block cannot be sliced and must be accessed as complete units using meta.get() instead.




**Parameters:**


* `key` Array key in store (must be separately stored with blocks) 
* `slices` Vector of slices, one per dimension Empty = entire dimension, unfilled dimensions = full slice 



**Returns:**

[**NDArray**](classstar_1_1NDArray.md) containing the requested slice 




**Exception:**


* `std::runtime_error` if key not found or stored in metadata block

Examples: // 1D: elements 1000-2000 (step defaults to 1) get\_slice&lt;double&gt;("large\_timeseries", {{1000, 2000}})


// 2D: rows 10-20, all columns get\_slice&lt;float&gt;("image\_data", {{10, 20}, {0, width}})


// Using helper functions for clarity get\_slice&lt;float&gt;("matrix", {slice\_range(10, 20), slice\_all(width)})


// 3D: hyperslab get\_slice&lt;uint16\_t&gt;("volume", {{0, 10}, {5, 15}, {0, depth}})


// For small arrays in metadata block, use meta.get() instead: auto small\_array = store.meta.get("small\_data"); 


        

<hr>



### function has\_layer 

_Check if layer exists._ 
```C++
inline bool star::StarDataset::has_layer (
    const std::string & layer_name
) const
```





**Parameters:**


* `layer_name` Layer name to check 



**Returns:**

true if layer exists, false otherwise 





        

<hr>



### function has\_metadata\_block 

_Checks if file has metadata block._ 
```C++
inline bool star::StarDataset::has_metadata_block () const
```





**Returns:**

True if metadata block exists 





        

<hr>



### function is\_metadata\_loaded 

_Checks if metadata block has been loaded._ 
```C++
inline bool star::StarDataset::is_metadata_loaded () const
```





**Returns:**

True if loaded 





        

<hr>



### function is\_read\_only 

_Check if file is in read-only mode._ 
```C++
inline bool star::StarDataset::is_read_only () const
```





**Returns:**

True if read-only 





        

<hr>



### function is\_sliceable 

_Checks if an array supports slicing._ 
```C++
inline bool star::StarDataset::is_sliceable (
    const std::string & key
) const
```



Arrays stored in the metadata block cannot be sliced - they must be accessed as complete units. Only separately stored arrays with block compression support efficient slicing.




**Parameters:**


* `key` Array key to check 



**Returns:**

True if array supports [**get\_slice()**](classstar_1_1StarDataset.md#function-get_slice), false if only meta.get() works


Example: if (store.is\_sliceable("large\_data")) { auto slice = store.get\_slice&lt;double&gt;("large\_data", {{0, 1000}}); } else { auto full = store.meta.get("small\_data"); } 


        

<hr>



### function key\_in\_layer 

_Check if key exists in specific layer using bit-mask (O(1))._ 
```C++
inline bool star::StarDataset::key_in_layer (
    const std::string & key,
    const std::string & layer_name
) const
```





**Parameters:**


* `key` Key to check 
* `layer_name` Layer name 



**Returns:**

true if key exists in layer, false otherwise 





        

<hr>



### function layer\_inheritance 

```C++
inline bool star::StarDataset::layer_inheritance () const
```




<hr>



### function list\_layers 

_Get list of all layer names._ 
```C++
inline std::vector< std::string > star::StarDataset::list_layers () const
```





**Returns:**

Vector of layer names 





        

<hr>



### function loadIndex 

_Loads the index from the file._ 
```C++
inline void star::StarDataset::loadIndex () 
```




<hr>



### function load\_all\_metadata 

_Load all layer metadata blocks at once (v3 format)._ 
```C++
inline void star::StarDataset::load_all_metadata () 
```




<hr>



### function load\_entry 

_Load entry from disk into memory._ 
```C++
inline void star::StarDataset::load_entry (
    size_t idx
) 
```





**Parameters:**


* `idx` Index in SoA arrays 




        

<hr>



### function load\_layer\_metadata 

_Load a specific layer's metadata block (v3 format)._ 
```C++
inline void star::StarDataset::load_layer_metadata (
    const std::string & layer_name
) 
```





**Parameters:**


* `layer_name` Name of the layer to load 




        

<hr>



### function load\_metadata\_block 

```C++
inline void star::StarDataset::load_metadata_block () 
```




<hr>



### function open\_options 

_Read-time options (e.g. layer inheritance) for this dataset._ 
```C++
inline const OpenOptions & star::StarDataset::open_options () const
```



These affect only how the in-memory dataset resolves reads; nothing is persisted. They are safe to change at any time, including on a read-only dataset, and take effect immediately for existing and future LayerViews. 


        

<hr>



### function operator= 

```C++
StarDataset & star::StarDataset::operator= (
    const StarDataset &
) = delete
```




<hr>



### function operator= 

```C++
StarDataset & star::StarDataset::operator= (
    StarDataset &&
) = delete
```




<hr>



### function parse\_layer\_metadata\_block 

_Parse a layer's metadata block (STARMeta format)._ 
```C++
inline void star::StarDataset::parse_layer_metadata_block (
    size_t layer_idx,
    const std::vector< char > & decompressed
) 
```





**Parameters:**


* `layer_idx` Layer index 
* `decompressed` Decompressed block data 




        

<hr>



### function prefetch 

_Load several arrays into cache in parallel (a read-ahead hint)._ 
```C++
inline void star::StarDataset::prefetch (
    const std::vector< std::string > & keys
) 
```



A single [**get()**](classstar_1_1StarDataset.md#function-get)/get\_slice() reads one array end-to-end: the read pipeline (ranged I/O -&gt; decompress -&gt; unshuffle -&gt; materialize) runs serially, and a caller reading N columns pays that latency N times back-to-back — only the block decompression WITHIN one array is threaded. [**prefetch()**](classstar_1_1StarDataset.md#function-prefetch) overlaps the whole pipeline ACROSS arrays:



* I/O stays SERIAL on the dataset's single persistent reader (one reused connection; remote reads issue exactly one GET per array, same as today — no request amplification), but
* each array's CPU work (decompress + unshuffle + build [**NDArray**](classstar_1_1NDArray.md)) is dispatched to the thread pool the moment its bytes arrive, so array k+1's read overlaps array k's decode and multiple decodes run at once.




After [**prefetch()**](classstar_1_1StarDataset.md#function-prefetch) returns, the named arrays are cached, so subsequent [**get&lt;T&gt;()**](classstar_1_1StarDataset.md#function-get)/get\_slice&lt;T&gt;() calls just copy out — this is the batch "read many
columns" path. Results are byte-identical to loading each key individually; this only changes WHEN/where the work runs. Keys that are unknown throw (same contract as get); keys already loaded or stored in the metadata block are handled on the normal serial path. With threading disabled (num\_threads == 1) it degrades to loading each key in turn.


Thread-safety: holds the dataset write lock for the whole batch; the decode tasks are pure (they read only stable per-entry metadata we do not mutate here and own their input buffers), and they pass nullptr for the pool so a pool worker never waits on the same pool (no nested-parallel deadlock). 


        

<hr>



### function print\_header 

```C++
inline void star::StarDataset::print_header () const
```




<hr>



### function put [1/2]

_Store array as separate compressed array (not in metadata block)._ 
```C++
template<typename T>
inline void star::StarDataset::put (
    const std::string & key,
    NDArray < T > && value
) 
```



Use this for large arrays that need slicing support. Small arrays should use meta.put() instead.




**Parameters:**


* `key` Array key 
* `value` [**NDArray**](classstar_1_1NDArray.md) to store 




        

<hr>



### function put [2/2]

_Store array as separate compressed array (const lvalue overload)._ 
```C++
template<typename T>
inline void star::StarDataset::put (
    const std::string & key,
    const NDArray < T > & value
) 
```




<hr>



### function read\_range 

_Read exactly [position, position+len) from the backing file via the persistent reader. Returns the bytes actually read._ 
```C++
inline std::vector< char > star::StarDataset::read_range (
    size_t position,
    size_t len
) 
```




<hr>



### function reader 

_Return the dataset's persistent byte-range reader, creating it on first use. One reader (= one reused connection for remote files) is shared by every read path — the index load, layer-metadata loads, and array-block reads — so remote reads never open a new connection or issue a HEAD per read._ 
```C++
inline RangeReader & star::StarDataset::reader () 
```



For small REMOTE files, this also triggers the whole-file prefetch (OpenOptions::prefetch\_whole\_below\_bytes): the object is fetched once and all subsequent reads are served from memory. 


        

<hr>



### function save\_to 

```C++
inline void star::StarDataset::save_to (
    const std::string & target_path
) 
```




<hr>



### function serialize\_array\_data [1/2]

_Serialize an_ [_**NDArray**_](classstar_1_1NDArray.md) _'s element data into a byte buffer for block storage, in the SAME wire format_[_**deserialize\_typed\_value()**_](classstar_1_1StarDataset.md#function-deserialize_typed_value) _reads._
```C++
template<typename T>
inline std::vector< char > star::StarDataset::serialize_array_data (
    const std::vector< T > & data
) const
```



Numeric types are raw contiguous bytes; std::string arrays are length-prefixed (uint32 total length, then per-element uint32 length + bytes). Using this everywhere fixes the bug where string arrays were memcpy'd as raw std::string OBJECTS (pointers/SSO) and came back empty after reload. For numeric types the bytes are identical to the old memcpy. 


        

<hr>



### function serialize\_array\_data [2/2]

_Serialize array data, applying the byte-shuffle prefilter if the codec is a \*\_SHUFFLE variant._ 
```C++
template<typename T>
inline std::vector< char > star::StarDataset::serialize_array_data (
    const std::vector< T > & data,
    CompressionAlgorithm codec,
    size_t block_size
) const
```



Shuffle is only valid for fixed-width numeric elements laid out as raw contiguous bytes; std::string arrays are length-prefixed and variable width, so they are never shuffled (they store fine under the base codec).


Two shuffle layouts are produced depending on the codec:
* global (GZIP\_SHUFFLE/LZ4\_SHUFFLE): shuffle the whole buffer at once.
* per-block (GZIP\_SHUFFLE\_BLOCK/LZ4\_SHUFFLE\_BLOCK): shuffle each `block_size` chunk independently, so it lines up with the compression blocks that compressBlocksBuffered() cuts at the same boundaries. Each block is then self-contained and sliceable. 




        

<hr>



### function serialize\_layer\_metadata\_block 

_Serializes a single layer's metadata in STARMeta format (v1)._ 
```C++
inline void star::StarDataset::serialize_layer_metadata_block (
    std::ostream & os,
    const std::string & layer_name
) 
```





**Parameters:**


* `os` Output stream 
* `layer_name` Layer name 




        

<hr>



### function serialize\_metadata\_block 

_Serializes metadata block to stream (OLD FORMAT - deprecated)._ 
```C++
inline void star::StarDataset::serialize_metadata_block (
    std::ostream & os
) const
```





**Parameters:**


* `os` Output stream 




        

<hr>



### function serialize\_metadata\_value [1/2]

_Helper to serialize metadata value (string specialization)._ 
```C++
template<typename T>
inline std::enable_if< std::is_same< T, std::string >::value, void >::type star::StarDataset::serialize_metadata_value (
    std::ostream & os,
    const std::vector< T > & data
) const
```




<hr>



### function serialize\_metadata\_value [2/2]

_Helper to serialize metadata value (numeric types)._ 
```C++
template<typename T>
inline std::enable_if<!std::is_same< T, std::string >::value, void >::type star::StarDataset::serialize_metadata_value (
    std::ostream & os,
    const std::vector< T > & data
) const
```




<hr>



### function set\_layer\_inheritance 

```C++
inline void star::StarDataset::set_layer_inheritance (
    bool on
) 
```




<hr>



### function set\_layer\_presence 

_Set layer presence for a data array key._ 
```C++
inline void star::StarDataset::set_layer_presence (
    const std::string & layer_name,
    const std::string & key,
    bool present
) 
```





**Parameters:**


* `layer_name` Layer name 
* `key` Data array key 
* `present` Whether the key is present in this layer 




        

<hr>



### function set\_open\_options 

```C++
inline void star::StarDataset::set_open_options (
    const OpenOptions & opts
) 
```




<hr>



### function shape\_of 

_Full shape (all dimensions) of a stored data array._ 
```C++
inline std::vector< size_t > star::StarDataset::shape_of (
    const std::string & key
) const
```



Metadata-only: the shape is read from the index loaded at [**open()**](classstar_1_1StarDataset.md#function-open-12) time, so this issues NO data read (no decompression, no extra network request for remote datasets). Complements [**dtype\_of()**](classstar_1_1StarDataset.md#function-dtype_of)/array\_length(); use [**get&lt;T&gt;()**](classstar_1_1StarDataset.md#function-get) only when the element data itself is needed.




**Parameters:**


* `key` Array key in store 



**Returns:**

Dimension sizes (empty for a scalar) 




**Exception:**


* `std::runtime_error` if the key is not a stored data array 




        

<hr>



### function size 

_Removes a key-value pair from the store._ 
```C++
inline size_t star::StarDataset::size () const
```





**Parameters:**


* `key` Key to remove

Returns the number of key-value pairs in the store 

**Returns:**

Number of key-value pairs 





        

<hr>



### function useThreading 

_Check if threading should be used based on workload._ 
```C++
inline bool star::StarDataset::useThreading (
    size_t num_blocks,
    size_t data_size
) const
```





**Parameters:**


* `num_blocks` Number of blocks to process 
* `data_size` Total data size in bytes 



**Returns:**

true if threading should be used, false otherwise 





        

<hr>



### function write\_bytes 

_Serialize the dataset to an in-memory byte buffer._ 
```C++
inline std::vector< char > star::StarDataset::write_bytes () 
```



The byte-array counterpart of save\_to(): returns a complete .stards image (the exact bytes that would be written to a file) instead of writing to a path. Works on any dataset — including read-only ones and datasets opened with [**open\_bytes()**](classstar_1_1StarDataset.md#function-open_bytes-12) — since it never touches the source file. Round-trips with [**open\_bytes()**](classstar_1_1StarDataset.md#function-open_bytes-12): `open_bytes (ds-> write_bytes() )` reconstructs the dataset.




**Returns:**

A complete .stards image as a byte array. 





        

<hr>



### function ~StarDataset 

_Destructor - writes all pending changes to disk (RAII)._ 
```C++
inline star::StarDataset::~StarDataset () 
```




<hr>
## Public Static Functions Documentation




### function create 

_Create a new Star dataset file._ 
```C++
static inline std::shared_ptr< StarDataset > star::StarDataset::create (
    const std::string & filename,
    const StarConfig & config=StarConfig ()
) 
```



Creates a new file with the specified configuration. If the file already exists, it will be overwritten. The file will be created on the first [**flush()**](classstar_1_1StarDataset.md#function-flush) or when the object is destroyed.




**Parameters:**


* `filename` Path to create (local, s3://... / /vsis3/...) 
* `config` Configuration for compression, block sizes, metadata 



**Returns:**

New [**StarDataset**](classstar_1_1StarDataset.md) instance 





        

<hr>



### function open [1/2]

_Open an existing Star dataset file._ 
```C++
static inline std::shared_ptr< StarDataset > star::StarDataset::open (
    const std::string & filename,
    FileMode mode=FileMode::READ_WRITE,
    const OpenOptions & opts={}
) 
```



Opens an existing file and reads its configuration from the file header. The configuration used when the file was created is preserved.


If mode is READ\_WRITE and file doesn't exist, it will be created. If mode is READ\_ONLY and file doesn't exist, an error is thrown.




**Parameters:**


* `filename` Path to open (local, s3:// or /vsis3/, [https://](https://) or /vsicurl/) 
* `mode` FileMode enum (READ\_WRITE/READ\_ONLY) 



**Returns:**

Opened [**StarDataset**](classstar_1_1StarDataset.md) instance 




**Exception:**


* `std::runtime_error` if file doesn't exist in READ\_ONLY mode or is corrupt 




        

<hr>



### function open [2/2]

_Open an existing Star dataset file (string mode overload)._ 
```C++
static inline std::shared_ptr< StarDataset > star::StarDataset::open (
    const std::string & filename,
    const std::string & mode_str,
    const OpenOptions & opts={}
) 
```





**Parameters:**


* `filename` Path to open (local, s3:// or /vsis3/, [https://](https://) or /vsicurl/) 
* `mode_str` String mode ("r", "w", "rw", "a") 



**Returns:**

Opened [**StarDataset**](classstar_1_1StarDataset.md) instance 




**Exception:**


* `std::runtime_error` if file doesn't exist or is invalid 




        

<hr>



### function open\_bytes [1/2]

_Open a dataset from an in-memory byte buffer._ 
```C++
static inline std::shared_ptr< StarDataset > star::StarDataset::open_bytes (
    std::vector< char > bytes,
    const OpenOptions & opts={}
) 
```



Mirrors [**open()**](classstar_1_1StarDataset.md#function-open-12), but the source is a byte array holding a complete .stards image (e.g. bytes received over a socket or pulled from a database) instead of a path. The dataset is READ\_ONLY — there is no backing file to flush to; use [**write\_bytes()**](classstar_1_1StarDataset.md#function-write_bytes) to serialize modifications back out to a new byte array.




**Parameters:**


* `bytes` A complete .stards image. 
* `opts` Read-time options (e.g. layer\_inheritance). 



**Returns:**

Opened [**StarDataset**](classstar_1_1StarDataset.md) backed by the provided bytes. 




**Exception:**


* `std::runtime_error` if the bytes are not a valid STAR image. 




        

<hr>



### function open\_bytes [2/2]

_Convenience overload taking a raw pointer + length (e.g. from a C buffer, Python bytes, or a mapped region)._ 
```C++
static inline std::shared_ptr< StarDataset > star::StarDataset::open_bytes (
    const void * data,
    size_t size,
    const OpenOptions & opts={}
) 
```




<hr>

------------------------------
The documentation for this class was generated from the following file `StarDS/include/stards.h`


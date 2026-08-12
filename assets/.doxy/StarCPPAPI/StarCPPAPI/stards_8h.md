

# File stards.h



[**FileList**](files.md) **>** [**include**](dir_b0f4f9828a575b9faf4f13364a06fab6.md) **>** [**stards.h**](stards_8h.md)

[Go to the source code of this file](stards_8h_source.md)



* `#include <algorithm>`
* `#include <cmath>`
* `#include <cstdint>`
* `#include <cstring>`
* `#include <fstream>`
* `#include <iomanip>`
* `#include <iostream>`
* `#include <memory>`
* `#include <numeric>`
* `#include <sstream>`
* `#include <stdexcept>`
* `#include <string>`
* `#include <type_traits>`
* `#include <map>`
* `#include <set>`
* `#include <unordered_map>`
* `#include <unordered_set>`
* `#include <variant>`
* `#include <vector>`
* `#include <chrono>`
* `#include <atomic>`
* `#include <shared_mutex>`
* `#include <mutex>`
* `#include <thread>`
* `#include <queue>`
* `#include <functional>`
* `#include <condition_variable>`
* `#include <future>`
* `#include <optional>`
* `#include <ctime>`
* `#include <tuple>`
* `#include <fcntl.h>`
* `#include <unistd.h>`
* `#include <zlib.h>`
* `#include <lz4.h>`
* `#include <curl/curl.h>`
* `#include <openssl/sha.h>`
* `#include <openssl/hmac.h>`
* `#include <dirent.h>`
* `#include <streambuf>`
* `#include <utility>`
* `#include <array>`













## Namespaces

| Type | Name |
| ---: | :--- |
| namespace | [**star**](namespacestar.md) <br> |
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
| struct | [**ElementRange**](structstar_1_1ExtractionPlan_1_1ElementRange.md) <br> |
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
| struct | [**HeaderData**](structstar_1_1S3Writer_1_1HeaderData.md) <br> |
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

















































## Macros

| Type | Name |
| ---: | :--- |
| define  | [**LOG\_DEBUG**](stards_8h.md#define-log_debug) (...) `do { if (logger::STARDS\_DEBUG &gt;= logger::current\_log\_level) logger::log\_internal(logger::STARDS\_DEBUG, \_\_LINE\_\_, \_\_func\_\_, \_\_VA\_ARGS\_\_); } while(0)`<br> |
| define  | [**LOG\_ERROR**](stards_8h.md#define-log_error) (...) `do { if (logger::STARDS\_ERROR &gt;= logger::current\_log\_level) logger::log\_internal(logger::STARDS\_ERROR, \_\_LINE\_\_, \_\_func\_\_, \_\_VA\_ARGS\_\_); } while(0)`<br> |
| define  | [**LOG\_INFO**](stards_8h.md#define-log_info) (...) `do { if (logger::STARDS\_INFO &gt;= logger::current\_log\_level) logger::log\_internal(logger::STARDS\_INFO, \_\_LINE\_\_, \_\_func\_\_, \_\_VA\_ARGS\_\_); } while(0)`<br> |
| define  | [**LOG\_TRACE**](stards_8h.md#define-log_trace) (...) `do { if (logger::STARDS\_TRACE &gt;= logger::current\_log\_level) logger::log\_internal(logger::STARDS\_TRACE, \_\_LINE\_\_, \_\_func\_\_, \_\_VA\_ARGS\_\_); } while(0)`<br> |
| define  | [**LOG\_WARN**](stards_8h.md#define-log_warn) (...) `do { if (logger::STARDS\_WARN &gt;= logger::current\_log\_level) logger::log\_internal(logger::STARDS\_WARN, \_\_LINE\_\_, \_\_func\_\_, \_\_VA\_ARGS\_\_); } while(0)`<br> |
| define  | [**STARDS\_IS\_BIG\_ENDIAN**](stards_8h.md#define-stards_is_big_endian)  `0`<br> |
| define  | [**STAR\_STRINGIFY**](stards_8h.md#define-star_stringify) (x) `STAR\_STRINGIFY\_IMPL(x)`<br> |
| define  | [**STAR\_STRINGIFY\_IMPL**](stards_8h.md#define-star_stringify_impl) (x) `#x`<br> |
| define  | [**STAR\_VERSION\_MAJOR**](stards_8h.md#define-star_version_major)  `1`<br> |
| define  | [**STAR\_VERSION\_MINOR**](stards_8h.md#define-star_version_minor)  `0`<br> |
| define  | [**STAR\_VERSION\_PATCH**](stards_8h.md#define-star_version_patch)  `0`<br> |
| define  | [**STAR\_VERSION\_STRING**](stards_8h.md#define-star_version_string)  `/* multi line expression */`<br> |

## Macro Definition Documentation





### define LOG\_DEBUG 

```C++
#define LOG_DEBUG (
    ...
) `do { if (logger::STARDS_DEBUG >= logger::current_log_level) logger::log_internal(logger::STARDS_DEBUG, __LINE__, __func__, __VA_ARGS__); } while(0)`
```




<hr>



### define LOG\_ERROR 

```C++
#define LOG_ERROR (
    ...
) `do { if (logger::STARDS_ERROR >= logger::current_log_level) logger::log_internal(logger::STARDS_ERROR, __LINE__, __func__, __VA_ARGS__); } while(0)`
```




<hr>



### define LOG\_INFO 

```C++
#define LOG_INFO (
    ...
) `do { if (logger::STARDS_INFO >= logger::current_log_level) logger::log_internal(logger::STARDS_INFO, __LINE__, __func__, __VA_ARGS__); } while(0)`
```




<hr>



### define LOG\_TRACE 

```C++
#define LOG_TRACE (
    ...
) `do { if (logger::STARDS_TRACE >= logger::current_log_level) logger::log_internal(logger::STARDS_TRACE, __LINE__, __func__, __VA_ARGS__); } while(0)`
```




<hr>



### define LOG\_WARN 

```C++
#define LOG_WARN (
    ...
) `do { if (logger::STARDS_WARN >= logger::current_log_level) logger::log_internal(logger::STARDS_WARN, __LINE__, __func__, __VA_ARGS__); } while(0)`
```




<hr>



### define STARDS\_IS\_BIG\_ENDIAN 

```C++
#define STARDS_IS_BIG_ENDIAN `0`
```




<hr>



### define STAR\_STRINGIFY 

```C++
#define STAR_STRINGIFY (
    x
) `STAR_STRINGIFY_IMPL(x)`
```




<hr>



### define STAR\_STRINGIFY\_IMPL 

```C++
#define STAR_STRINGIFY_IMPL (
    x
) `#x`
```




<hr>



### define STAR\_VERSION\_MAJOR 

```C++
#define STAR_VERSION_MAJOR `1`
```




<hr>



### define STAR\_VERSION\_MINOR 

```C++
#define STAR_VERSION_MINOR `0`
```




<hr>



### define STAR\_VERSION\_PATCH 

```C++
#define STAR_VERSION_PATCH `0`
```




<hr>



### define STAR\_VERSION\_STRING 

```C++
#define STAR_VERSION_STRING `/* multi line expression */`
```




<hr>

------------------------------
The documentation for this class was generated from the following file `StarDS/include/stards.h`


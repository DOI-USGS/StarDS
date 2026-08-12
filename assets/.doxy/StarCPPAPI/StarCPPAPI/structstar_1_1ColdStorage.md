

# Struct star::ColdStorage



[**ClassList**](annotated.md) **>** [**star**](namespacestar.md) **>** [**ColdStorage**](structstar_1_1ColdStorage.md)



_Cold storage - infrequently accessed data._ [More...](#detailed-description)

* `#include <stards.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  std::vector&lt; std::vector&lt; [**BlockInfo**](structstar_1_1BlockInfo.md) &gt; &gt; | [**block\_infos**](#variable-block_infos)  <br> |
|  std::vector&lt; uint32\_t &gt; | [**compressed\_sizes**](#variable-compressed_sizes)  <br> |
|  std::vector&lt; CompressionAlgorithm &gt; | [**compressions**](#variable-compressions)  <br> |
|  std::vector&lt; uint64\_t &gt; | [**file\_positions**](#variable-file_positions)  <br> |
|  std::vector&lt; std::vector&lt; size\_t &gt; &gt; | [**shapes**](#variable-shapes)  <br> |
|  std::vector&lt; uint8\_t &gt; | [**stored\_in\_metadata\_flags**](#variable-stored_in_metadata_flags)  <br> |
|  std::vector&lt; uint32\_t &gt; | [**uncompressed\_sizes**](#variable-uncompressed_sizes)  <br> |












































## Detailed Description


These fields are only accessed during flush/serialization. Separated from hot data to avoid cache pollution. 


    
## Public Attributes Documentation




### variable block\_infos 

```C++
std::vector<std::vector<BlockInfo> > star::ColdStorage::block_infos;
```




<hr>



### variable compressed\_sizes 

```C++
std::vector<uint32_t> star::ColdStorage::compressed_sizes;
```




<hr>



### variable compressions 

```C++
std::vector<CompressionAlgorithm> star::ColdStorage::compressions;
```




<hr>



### variable file\_positions 

```C++
std::vector<uint64_t> star::ColdStorage::file_positions;
```




<hr>



### variable shapes 

```C++
std::vector<std::vector<size_t> > star::ColdStorage::shapes;
```




<hr>



### variable stored\_in\_metadata\_flags 

```C++
std::vector<uint8_t> star::ColdStorage::stored_in_metadata_flags;
```




<hr>



### variable uncompressed\_sizes 

```C++
std::vector<uint32_t> star::ColdStorage::uncompressed_sizes;
```




<hr>

------------------------------
The documentation for this class was generated from the following file `StarDS/include/stards.h`


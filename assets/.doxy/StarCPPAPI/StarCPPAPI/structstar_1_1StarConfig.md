

# Struct star::StarConfig



[**ClassList**](annotated.md) **>** [**star**](namespacestar.md) **>** [**StarConfig**](structstar_1_1StarConfig.md)



_Configuration for metadata block optimization._ [More...](#detailed-description)

* `#include <stards.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  size\_t | [**arena\_chunk\_size**](#variable-arena_chunk_size)   = `1 \* 1024 \* 1024`<br> |
|  size\_t | [**block\_size**](#variable-block_size)   = `1024 \* 1024`<br> |
|  size\_t | [**buffer\_shrink\_threshold**](#variable-buffer_shrink_threshold)   = `1024 \* 1024`<br> |
|  CompressionAlgorithm | [**compression**](#variable-compression)   = `/* multi line expression */`<br> |
|  bool | [**metadata\_block\_enabled**](#variable-metadata_block_enabled)   = `true`<br> |
|  CompressionAlgorithm | [**metadata\_compression**](#variable-metadata_compression)   = `/* multi line expression */`<br> |
|  std::set&lt; std::string &gt; | [**metadata\_force\_separate\_keys**](#variable-metadata_force_separate_keys)  <br> |
|  size\_t | [**metadata\_max\_block\_size**](#variable-metadata_max_block_size)   = `64 \* 1024`<br> |












































## Detailed Description


The metadata block system groups small scalar/array values into a single compressed block to reduce overhead and cloud access requests. 


    
## Public Attributes Documentation




### variable arena\_chunk\_size 

```C++
size_t star::StarConfig::arena_chunk_size;
```




<hr>



### variable block\_size 

```C++
size_t star::StarConfig::block_size;
```




<hr>



### variable buffer\_shrink\_threshold 

```C++
size_t star::StarConfig::buffer_shrink_threshold;
```




<hr>



### variable compression 

```C++
CompressionAlgorithm star::StarConfig::compression;
```




<hr>



### variable metadata\_block\_enabled 

```C++
bool star::StarConfig::metadata_block_enabled;
```




<hr>



### variable metadata\_compression 

```C++
CompressionAlgorithm star::StarConfig::metadata_compression;
```




<hr>



### variable metadata\_force\_separate\_keys 

```C++
std::set<std::string> star::StarConfig::metadata_force_separate_keys;
```




<hr>



### variable metadata\_max\_block\_size 

```C++
size_t star::StarConfig::metadata_max_block_size;
```




<hr>

------------------------------
The documentation for this class was generated from the following file `StarDS/include/stards.h`


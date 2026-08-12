

# Struct star::BlockMap



[**ClassList**](annotated.md) **>** [**star**](namespacestar.md) **>** [**BlockMap**](structstar_1_1BlockMap.md)



_Maps logical elements to physical blocks._ 

* `#include <stards.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  std::vector&lt; size\_t &gt; | [**block\_indices**](#variable-block_indices)  <br> |
|  std::vector&lt; size\_t &gt; | [**block\_offsets**](#variable-block_offsets)  <br> |
|  std::vector&lt; size\_t &gt; | [**block\_sizes**](#variable-block_sizes)  <br> |
|  bool | [**blocks\_contiguous**](#variable-blocks_contiguous)  <br> |
|  size\_t | [**contiguous\_start\_offset**](#variable-contiguous_start_offset)  <br> |
|  size\_t | [**total\_compressed\_bytes**](#variable-total_compressed_bytes)  <br> |












































## Public Attributes Documentation




### variable block\_indices 

```C++
std::vector<size_t> star::BlockMap::block_indices;
```




<hr>



### variable block\_offsets 

```C++
std::vector<size_t> star::BlockMap::block_offsets;
```




<hr>



### variable block\_sizes 

```C++
std::vector<size_t> star::BlockMap::block_sizes;
```




<hr>



### variable blocks\_contiguous 

```C++
bool star::BlockMap::blocks_contiguous;
```




<hr>



### variable contiguous\_start\_offset 

```C++
size_t star::BlockMap::contiguous_start_offset;
```




<hr>



### variable total\_compressed\_bytes 

```C++
size_t star::BlockMap::total_compressed_bytes;
```




<hr>

------------------------------
The documentation for this class was generated from the following file `StarDS/include/stards.h`


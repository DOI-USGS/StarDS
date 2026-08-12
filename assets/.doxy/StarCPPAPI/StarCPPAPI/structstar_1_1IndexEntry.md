

# Struct star::IndexEntry



[**ClassList**](annotated.md) **>** [**star**](namespacestar.md) **>** [**IndexEntry**](structstar_1_1IndexEntry.md)



_Index entry with block compression support and shape information._ [More...](#detailed-description)

* `#include <stards.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  uint64\_t | [**block\_size**](#variable-block_size)  <br> |
|  std::vector&lt; [**BlockInfo**](structstar_1_1BlockInfo.md) &gt; | [**blocks**](#variable-blocks)  <br> |
|  CompressionAlgorithm | [**compression**](#variable-compression)  <br> |
|  DataType | [**datatype**](#variable-datatype)  <br> |
|  bool | [**dirty**](#variable-dirty)  <br> |
|  bool | [**is\_metadata\_block**](#variable-is_metadata_block)   = `false`<br> |
|  uint64\_t | [**position**](#variable-position)  <br> |
|  std::vector&lt; size\_t &gt; | [**shape**](#variable-shape)  <br> |
|  bool | [**stored\_in\_metadata**](#variable-stored_in_metadata)   = `false`<br> |
|  uint64\_t | [**total\_bytes**](#variable-total_bytes)  <br> |
















## Public Functions

| Type | Name |
| ---: | :--- |
|  bool | [**is\_scalar**](#function-is_scalar) () const<br> |
|  size\_t | [**num\_elements**](#function-num_elements) () const<br> |
|  void | [**print**](#function-print) () const<br> |
|  void | [**read**](#function-read) (std::istream & is) <br> |
|  size\_t | [**serialized\_size**](#function-serialized_size) () const<br> |
|  void | [**write**](#function-write) (std::ostream & os) const<br> |




























## Detailed Description




**Note:**

On-disk portability: this descriptor and its [**BlockInfo**](structstar_1_1BlockInfo.md) entries serialize their size\_t fields as fixed-width little-endian uint64 values via write\_u64 / read\_u64 (which use the shift-based write\_le/read\_le, independent of host width and byte order). So .stards files interchange freely across platforms of any pointer width or endianness, including 32-bit wasm32 (size\_t == 4 bytes) — the fields are narrowed to/from size\_t with explicit static\_casts on read. This matches the documented format-v1 layout. 





    
## Public Attributes Documentation




### variable block\_size 

```C++
uint64_t star::IndexEntry::block_size;
```




<hr>



### variable blocks 

```C++
std::vector<BlockInfo> star::IndexEntry::blocks;
```




<hr>



### variable compression 

```C++
CompressionAlgorithm star::IndexEntry::compression;
```




<hr>



### variable datatype 

```C++
DataType star::IndexEntry::datatype;
```




<hr>



### variable dirty 

```C++
bool star::IndexEntry::dirty;
```




<hr>



### variable is\_metadata\_block 

```C++
bool star::IndexEntry::is_metadata_block;
```




<hr>



### variable position 

```C++
uint64_t star::IndexEntry::position;
```




<hr>



### variable shape 

```C++
std::vector<size_t> star::IndexEntry::shape;
```




<hr>



### variable stored\_in\_metadata 

```C++
bool star::IndexEntry::stored_in_metadata;
```




<hr>



### variable total\_bytes 

```C++
uint64_t star::IndexEntry::total_bytes;
```




<hr>
## Public Functions Documentation




### function is\_scalar 

```C++
inline bool star::IndexEntry::is_scalar () const
```




<hr>



### function num\_elements 

```C++
inline size_t star::IndexEntry::num_elements () const
```




<hr>



### function print 

```C++
inline void star::IndexEntry::print () const
```




<hr>



### function read 

```C++
inline void star::IndexEntry::read (
    std::istream & is
) 
```




<hr>



### function serialized\_size 

```C++
inline size_t star::IndexEntry::serialized_size () const
```




<hr>



### function write 

```C++
inline void star::IndexEntry::write (
    std::ostream & os
) const
```




<hr>

------------------------------
The documentation for this class was generated from the following file `StarDS/include/stards.h`


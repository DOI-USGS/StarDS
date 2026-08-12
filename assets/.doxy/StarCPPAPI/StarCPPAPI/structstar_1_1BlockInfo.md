

# Struct star::BlockInfo



[**ClassList**](annotated.md) **>** [**star**](namespacestar.md) **>** [**BlockInfo**](structstar_1_1BlockInfo.md)



_Metadata for a single compressed block._ 

* `#include <stards.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  uint64\_t | [**compressed\_size**](#variable-compressed_size)  <br> |
|  uint64\_t | [**offset**](#variable-offset)  <br> |
|  uint64\_t | [**uncompressed\_size**](#variable-uncompressed_size)  <br> |
















## Public Functions

| Type | Name |
| ---: | :--- |
|  void | [**read**](#function-read) (std::istream & is) <br> |
|  void | [**write**](#function-write) (std::ostream & os) const<br> |




























## Public Attributes Documentation




### variable compressed\_size 

```C++
uint64_t star::BlockInfo::compressed_size;
```




<hr>



### variable offset 

```C++
uint64_t star::BlockInfo::offset;
```




<hr>



### variable uncompressed\_size 

```C++
uint64_t star::BlockInfo::uncompressed_size;
```




<hr>
## Public Functions Documentation




### function read 

```C++
inline void star::BlockInfo::read (
    std::istream & is
) 
```




<hr>



### function write 

```C++
inline void star::BlockInfo::write (
    std::ostream & os
) const
```




<hr>

------------------------------
The documentation for this class was generated from the following file `StarDS/include/stards.h`


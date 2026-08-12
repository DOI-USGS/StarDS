

# Struct star::FileHeader



[**ClassList**](annotated.md) **>** [**star**](namespacestar.md) **>** [**FileHeader**](structstar_1_1FileHeader.md)



_File header structure (31 bytes fixed size)._ [More...](#detailed-description)

* `#include <stards.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  uint64\_t | [**entry\_count**](#variable-entry_count)   = `0`<br> |
|  uint8\_t | [**format\_version**](#variable-format_version)   = `1`<br> |
|  uint64\_t | [**header\_size**](#variable-header_size)   = `0`<br> |
|  uint32\_t | [**key\_registry\_count**](#variable-key_registry_count)   = `0`<br> |
|  uint32\_t | [**layer\_count**](#variable-layer_count)   = `0`<br> |
|  char | [**magic**](#variable-magic)   = `{'S','T','A','R','D','S'}`<br> |
















## Public Functions

| Type | Name |
| ---: | :--- |
|  std::string | [**getVersionString**](#function-getversionstring) () const<br>_Get format version string._  |
|  bool | [**isValid**](#function-isvalid) () const<br>_Check if magic string is valid._  |
|  void | [**read**](#function-read) (std::istream & is) <br>_Read header from stream._  |
|  void | [**write**](#function-write) (std::ostream & os) const<br>_Write header to stream._  |


## Public Static Functions

| Type | Name |
| ---: | :--- |
|  size\_t | [**size**](#function-size) () <br>_Get fixed size of_ [_**FileHeader**_](structstar_1_1FileHeader.md) _struct._ |


























## Detailed Description


Layout:
* magic[6]: "STARDS" magic string (6 bytes)
* format\_version: File format version (1 byte, currently 1)
* header\_size: Total size of header + index section (8 bytes)
* entry\_count: Number of entries in the index (8 bytes)
* layer\_count: Number of named layers, excluding **base** (4 bytes)
* key\_registry\_count: Number of keys in the global key registry (4 bytes)




Note: Software/library version is NOT stored in the file, only the format version. 


    
## Public Attributes Documentation




### variable entry\_count 

```C++
uint64_t star::FileHeader::entry_count;
```




<hr>



### variable format\_version 

```C++
uint8_t star::FileHeader::format_version;
```




<hr>



### variable header\_size 

```C++
uint64_t star::FileHeader::header_size;
```




<hr>



### variable key\_registry\_count 

```C++
uint32_t star::FileHeader::key_registry_count;
```




<hr>



### variable layer\_count 

```C++
uint32_t star::FileHeader::layer_count;
```




<hr>



### variable magic 

```C++
char star::FileHeader::magic[MAGIC_STRING_LENGTH];
```




<hr>
## Public Functions Documentation




### function getVersionString 

_Get format version string._ 
```C++
inline std::string star::FileHeader::getVersionString () const
```




<hr>



### function isValid 

_Check if magic string is valid._ 
```C++
inline bool star::FileHeader::isValid () const
```




<hr>



### function read 

_Read header from stream._ 
```C++
inline void star::FileHeader::read (
    std::istream & is
) 
```




<hr>



### function write 

_Write header to stream._ 
```C++
inline void star::FileHeader::write (
    std::ostream & os
) const
```




<hr>
## Public Static Functions Documentation




### function size 

_Get fixed size of_ [_**FileHeader**_](structstar_1_1FileHeader.md) _struct._
```C++
static inline size_t star::FileHeader::size () 
```




<hr>

------------------------------
The documentation for this class was generated from the following file `StarDS/include/stards.h`


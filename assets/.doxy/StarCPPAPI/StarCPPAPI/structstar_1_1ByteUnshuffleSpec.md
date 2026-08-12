

# Struct star::ByteUnshuffleSpec



[**ClassList**](annotated.md) **>** [**star**](namespacestar.md) **>** [**ByteUnshuffleSpec**](structstar_1_1ByteUnshuffleSpec.md)



_Describes an in-place byte-unshuffle to fuse into an array's fill._ [More...](#detailed-description)

* `#include <stards.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  bool | [**active**](#variable-active)   = `false`<br> |
|  size\_t | [**block\_size**](#variable-block_size)   = `0`<br> |
|  bool | [**blocked**](#variable-blocked)   = `false`<br> |
|  size\_t | [**elem\_size**](#variable-elem_size)   = `0`<br> |












































## Detailed Description


On the read hot path the decompressed bytes of a shuffle-codec array are byte-planes that must be transposed back to element order before use. Rather than unshuffle into a scratch buffer and then memcpy that buffer into the [**NDArray**](classstar_1_1NDArray.md) (two full passes), deserialize\_typed\_value\_bytes() can unshuffle straight into the freshly allocated [**NDArray**](classstar_1_1NDArray.md) storage (one pass) when handed one of these. `active == false` means "no shuffle — plain memcpy", preserving the exact behavior of the non-shuffle codecs. 


    
## Public Attributes Documentation




### variable active 

```C++
bool star::ByteUnshuffleSpec::active;
```




<hr>



### variable block\_size 

```C++
size_t star::ByteUnshuffleSpec::block_size;
```




<hr>



### variable blocked 

```C++
bool star::ByteUnshuffleSpec::blocked;
```




<hr>



### variable elem\_size 

```C++
size_t star::ByteUnshuffleSpec::elem_size;
```




<hr>

------------------------------
The documentation for this class was generated from the following file `StarDS/include/stards.h`


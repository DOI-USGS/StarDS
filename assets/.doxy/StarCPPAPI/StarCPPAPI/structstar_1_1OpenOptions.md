

# Struct star::OpenOptions



[**ClassList**](annotated.md) **>** [**star**](namespacestar.md) **>** [**OpenOptions**](structstar_1_1OpenOptions.md)



_Read-time options for_ [_**StarDataset::open()**_](classstar_1_1StarDataset.md#function-open-12) _._[More...](#detailed-description)

* `#include <stards.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  bool | [**layer\_inheritance**](#variable-layer_inheritance)   = `false`<br> |
|  size\_t | [**prefetch\_whole\_below\_bytes**](#variable-prefetch_whole_below_bytes)   = `8u \* 1024u \* 1024u`<br> |












































## Detailed Description


Distinct from [**StarConfig**](structstar_1_1StarConfig.md), which configures how a NEW dataset is _written_ by create() (compression, block sizes). [**OpenOptions**](structstar_1_1OpenOptions.md) only affects how an existing dataset is _read_; nothing here is persisted to the .stards file, so it is a per-open setting (and can also be changed after open — see StarDataset::set\_layer\_inheritance / set\_open\_options). 


    
## Public Attributes Documentation




### variable layer\_inheritance 

```C++
bool star::OpenOptions::layer_inheritance;
```




<hr>



### variable prefetch\_whole\_below\_bytes 

```C++
size_t star::OpenOptions::prefetch_whole_below_bytes;
```




<hr>

------------------------------
The documentation for this class was generated from the following file `StarDS/include/stards.h`


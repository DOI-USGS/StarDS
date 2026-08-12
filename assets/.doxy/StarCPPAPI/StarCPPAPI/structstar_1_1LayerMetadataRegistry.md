

# Struct star::LayerMetadataRegistry



[**ClassList**](annotated.md) **>** [**star**](namespacestar.md) **>** [**LayerMetadataRegistry**](structstar_1_1LayerMetadataRegistry.md)



_Per-layer metadata registry using data-oriented design (Structure of Arrays)._ [More...](#detailed-description)

* `#include <stards.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  std::vector&lt; uint64\_t &gt; | [**block\_positions**](#variable-block_positions)  <br> |
|  std::vector&lt; uint32\_t &gt; | [**block\_sizes**](#variable-block_sizes)  <br> |
|  std::vector&lt; CompressionAlgorithm &gt; | [**compressions**](#variable-compressions)  <br> |
|  std::vector&lt; std::unordered\_set&lt; uint16\_t &gt; &gt; | [**key\_indices**](#variable-key_indices)  <br> |
|  std::vector&lt; std::string &gt; | [**layer\_names**](#variable-layer_names)  <br> |
|  std::unordered\_map&lt; std::string, size\_t &gt; | [**name\_to\_layer\_index**](#variable-name_to_layer_index)  <br> |
















## Public Functions

| Type | Name |
| ---: | :--- |
|  size\_t | [**add\_layer**](#function-add_layer) (const std::string & layer\_name) <br>_Add a new layer to the registry._  |
|  bool | [**contains\_layer**](#function-contains_layer) (const std::string & layer\_name) const<br>_Check if layer exists._  |
|  size\_t | [**get\_layer\_index**](#function-get_layer_index) (const std::string & layer\_name) const<br>_Get layer index by name._  |




























## Detailed Description


Stores metadata about each layer's metadata block for lazy loading. Each layer has its own independent STARMeta block in the file. 


    
## Public Attributes Documentation




### variable block\_positions 

```C++
std::vector<uint64_t> star::LayerMetadataRegistry::block_positions;
```




<hr>



### variable block\_sizes 

```C++
std::vector<uint32_t> star::LayerMetadataRegistry::block_sizes;
```




<hr>



### variable compressions 

```C++
std::vector<CompressionAlgorithm> star::LayerMetadataRegistry::compressions;
```




<hr>



### variable key\_indices 

```C++
std::vector<std::unordered_set<uint16_t> > star::LayerMetadataRegistry::key_indices;
```




<hr>



### variable layer\_names 

```C++
std::vector<std::string> star::LayerMetadataRegistry::layer_names;
```




<hr>



### variable name\_to\_layer\_index 

```C++
std::unordered_map<std::string, size_t> star::LayerMetadataRegistry::name_to_layer_index;
```




<hr>
## Public Functions Documentation




### function add\_layer 

_Add a new layer to the registry._ 
```C++
inline size_t star::LayerMetadataRegistry::add_layer (
    const std::string & layer_name
) 
```




<hr>



### function contains\_layer 

_Check if layer exists._ 
```C++
inline bool star::LayerMetadataRegistry::contains_layer (
    const std::string & layer_name
) const
```




<hr>



### function get\_layer\_index 

_Get layer index by name._ 
```C++
inline size_t star::LayerMetadataRegistry::get_layer_index (
    const std::string & layer_name
) const
```





**Returns:**

size\_t index into the registry 





        

<hr>

------------------------------
The documentation for this class was generated from the following file `StarDS/include/stards.h`


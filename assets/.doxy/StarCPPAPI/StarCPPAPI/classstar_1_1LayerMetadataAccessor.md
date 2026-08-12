

# Class star::LayerMetadataAccessor



[**ClassList**](annotated.md) **>** [**star**](namespacestar.md) **>** [**LayerMetadataAccessor**](classstar_1_1LayerMetadataAccessor.md)



_Metadata accessor for a specific layer with inheritance._ [More...](#detailed-description)

* `#include <stards.h>`





































## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**LayerMetadataAccessor**](#function-layermetadataaccessor) ([**StarDataset**](classstar_1_1StarDataset.md) \* store, const std::string & layer\_name) <br> |
|  bool | [**contains**](#function-contains) (const std::string & key) const<br> |
|  std::shared\_ptr&lt; [**MetadataValue**](structstar_1_1MetadataValue.md) &gt; | [**get**](#function-get) (const std::string & key) <br> |
|  std::vector&lt; std::string &gt; | [**keys**](#function-keys) () const<br> |
|  void | [**put**](#function-put) (const std::string & key, const V & value) <br> |
|  void | [**remove**](#function-remove) (const std::string & key) <br> |




























## Detailed Description


Provides layer-specific metadata operations with automatic fallback to base layer. When getting metadata, checks layer first, then falls back to base if not found. When putting metadata, stores in layer-specific namespace. 


    
## Public Functions Documentation




### function LayerMetadataAccessor 

```C++
inline star::LayerMetadataAccessor::LayerMetadataAccessor (
    StarDataset * store,
    const std::string & layer_name
) 
```




<hr>



### function contains 

```C++
inline bool star::LayerMetadataAccessor::contains (
    const std::string & key
) const
```




<hr>



### function get 

```C++
inline std::shared_ptr< MetadataValue > star::LayerMetadataAccessor::get (
    const std::string & key
) 
```




<hr>



### function keys 

```C++
inline std::vector< std::string > star::LayerMetadataAccessor::keys () const
```




<hr>



### function put 

```C++
template<typename V>
inline void star::LayerMetadataAccessor::put (
    const std::string & key,
    const V & value
) 
```




<hr>



### function remove 

```C++
inline void star::LayerMetadataAccessor::remove (
    const std::string & key
) 
```




<hr>

------------------------------
The documentation for this class was generated from the following file `StarDS/include/stards.h`


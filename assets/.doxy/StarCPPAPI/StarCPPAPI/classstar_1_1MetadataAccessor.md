

# Class star::MetadataAccessor



[**ClassList**](annotated.md) **>** [**star**](namespacestar.md) **>** [**MetadataAccessor**](classstar_1_1MetadataAccessor.md)



_Accessor for metadata operations._ [More...](#detailed-description)

* `#include <stards.h>`





































## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**MetadataAccessor**](#function-metadataaccessor) (StarDataset \* store) <br> |
|  void | [**clear**](#function-clear) () <br> |
|  bool | [**contains**](#function-contains) (const std::string & key) const<br> |
|  std::shared\_ptr&lt; [**MetadataValue**](structstar_1_1MetadataValue.md) &gt; | [**get**](#function-get) (const std::string & key) <br> |
|  std::map&lt; std::string, [**MetadataValue**](structstar_1_1MetadataValue.md) &gt; | [**get\_all**](#function-get_all) () <br> |
|  std::map&lt; std::string, [**MetadataValue**](structstar_1_1MetadataValue.md) &gt; | [**get\_batch**](#function-get_batch) (const std::vector&lt; std::string &gt; & keys) <br> |
|  void | [**put**](#function-put) (const std::string & key, const V & value) <br> |
|  void | [**put\_batch**](#function-put_batch) (const std::map&lt; std::string, V &gt; & values) <br> |
|  void | [**remove**](#function-remove) (const std::string & key) <br> |




























## Detailed Description


Provides explicit metadata API via store.meta.put() and store.meta.get() for clear intent when working with metadata block items. 


    
## Public Functions Documentation




### function MetadataAccessor 

```C++
inline explicit star::MetadataAccessor::MetadataAccessor (
    StarDataset * store
) 
```




<hr>



### function clear 

```C++
inline void star::MetadataAccessor::clear () 
```




<hr>



### function contains 

```C++
inline bool star::MetadataAccessor::contains (
    const std::string & key
) const
```




<hr>



### function get 

```C++
inline std::shared_ptr< MetadataValue > star::MetadataAccessor::get (
    const std::string & key
) 
```




<hr>



### function get\_all 

```C++
inline std::map< std::string, MetadataValue > star::MetadataAccessor::get_all () 
```




<hr>



### function get\_batch 

```C++
inline std::map< std::string, MetadataValue > star::MetadataAccessor::get_batch (
    const std::vector< std::string > & keys
) 
```




<hr>



### function put 

```C++
template<typename V>
void star::MetadataAccessor::put (
    const std::string & key,
    const V & value
) 
```




<hr>



### function put\_batch 

```C++
template<typename V>
void star::MetadataAccessor::put_batch (
    const std::map< std::string, V > & values
) 
```




<hr>



### function remove 

```C++
inline void star::MetadataAccessor::remove (
    const std::string & key
) 
```




<hr>

------------------------------
The documentation for this class was generated from the following file `StarDS/include/stards.h`




# Class star::LayerView



[**ClassList**](annotated.md) **>** [**star**](namespacestar.md) **>** [**LayerView**](classstar_1_1LayerView.md)



_Lightweight view into a specific layer with inheritance from base._ [More...](#detailed-description)

* `#include <stards.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  [**LayerMetadataAccessor**](classstar_1_1LayerMetadataAccessor.md) | [**meta**](#variable-meta)  <br> |
















## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**LayerView**](#function-layerview) (std::shared\_ptr&lt; [**StarDataset**](classstar_1_1StarDataset.md) &gt; base, const std::string & layer\_name) <br> |
|  std::shared\_ptr&lt; [**StarDataset**](classstar_1_1StarDataset.md) &gt; | [**base**](#function-base) () const<br>_Get parent dataset._  |
|  bool | [**contains**](#function-contains) (const std::string & key) const<br>_Check if key exists in this layer or base (with inheritance)._  |
|  [**NDArray**](classstar_1_1NDArray.md)&lt; T &gt; | [**get**](#function-get) (const std::string & key) <br>_Get array from this layer with inheritance._  |
|  std::vector&lt; std::string &gt; | [**keys**](#function-keys) () const<br>_Get all keys in this layer (local + inherited)._  |
|  std::string | [**name**](#function-name) () const<br>_Get layer name._  |
|  void | [**put**](#function-put) (const std::string & key, [**NDArray**](classstar_1_1NDArray.md)&lt; T &gt; && value) <br>_Store array in this layer._  |




























## Detailed Description


[**LayerView**](classstar_1_1LayerView.md) provides the same API as [**StarDataset**](classstar_1_1StarDataset.md) but operates on a specific layer. Keys not found in the layer automatically fall back to the base layer.


Example: auto ds = [**StarDataset::open**](classstar_1_1StarDataset.md#function-open-12)("file.stards"); auto layer1 = ds-&gt;create\_layer("band\_0"); layer1-&gt;meta.put("wavelength", [**NDArray&lt;double&gt;**](classstar_1_1NDArray.md)({}, 450.5)); // Layer-specific auto inst = layer1-&gt;meta.get("instrument"); // Falls back to base 


    
## Public Attributes Documentation




### variable meta 

```C++
LayerMetadataAccessor star::LayerView::meta;
```




<hr>
## Public Functions Documentation




### function LayerView 

```C++
inline star::LayerView::LayerView (
    std::shared_ptr< StarDataset > base,
    const std::string & layer_name
) 
```




<hr>



### function base 

_Get parent dataset._ 
```C++
inline std::shared_ptr< StarDataset > star::LayerView::base () const
```





**Returns:**

Shared pointer to base [**StarDataset**](classstar_1_1StarDataset.md) 





        

<hr>



### function contains 

_Check if key exists in this layer or base (with inheritance)._ 
```C++
inline bool star::LayerView::contains (
    const std::string & key
) const
```





**Parameters:**


* `key` Key to check 



**Returns:**

true if key exists in layer or base 





        

<hr>



### function get 

_Get array from this layer with inheritance._ 
```C++
template<typename T>
NDArray < T > star::LayerView::get (
    const std::string & key
) 
```





**Template parameters:**


* `T` Element type 



**Parameters:**


* `key` Key to retrieve 



**Returns:**

[**NDArray**](classstar_1_1NDArray.md) with data from layer or base 





        

<hr>



### function keys 

_Get all keys in this layer (local + inherited)._ 
```C++
inline std::vector< std::string > star::LayerView::keys () const
```





**Returns:**

Vector of key names 





        

<hr>



### function name 

_Get layer name._ 
```C++
inline std::string star::LayerView::name () const
```





**Returns:**

Layer name 





        

<hr>



### function put 

_Store array in this layer._ 
```C++
template<typename T>
void star::LayerView::put (
    const std::string & key,
    NDArray < T > && value
) 
```





**Template parameters:**


* `T` Element type 



**Parameters:**


* `key` Key to store 
* `value` Data to store 




        

<hr>

------------------------------
The documentation for this class was generated from the following file `StarDS/include/stards.h`


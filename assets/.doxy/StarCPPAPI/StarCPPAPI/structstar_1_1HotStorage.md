

# Struct star::HotStorage



[**ClassList**](annotated.md) **>** [**star**](namespacestar.md) **>** [**HotStorage**](structstar_1_1HotStorage.md)



_Hot storage - frequently accessed data (cache-friendly)._ [More...](#detailed-description)

* `#include <stards.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  std::vector&lt; size\_t &gt; | [**data\_indices**](#variable-data_indices)  <br> |
|  std::vector&lt; bool &gt; | [**dirty\_flags**](#variable-dirty_flags)  <br> |
|  std::vector&lt; DataType &gt; | [**dtypes**](#variable-dtypes)  <br> |
|  std::vector&lt; std::string &gt; | [**keys**](#variable-keys)  <br> |
|  std::vector&lt; bool &gt; | [**loaded\_flags**](#variable-loaded_flags)  <br> |
|  std::vector&lt; StorageLocation &gt; | [**locations**](#variable-locations)  <br> |












































## Detailed Description


These fields are accessed during every get/put/contains operation. Packed together for better cache locality. 


    
## Public Attributes Documentation




### variable data\_indices 

```C++
std::vector<size_t> star::HotStorage::data_indices;
```




<hr>



### variable dirty\_flags 

```C++
std::vector<bool> star::HotStorage::dirty_flags;
```




<hr>



### variable dtypes 

```C++
std::vector<DataType> star::HotStorage::dtypes;
```




<hr>



### variable keys 

```C++
std::vector<std::string> star::HotStorage::keys;
```




<hr>



### variable loaded\_flags 

```C++
std::vector<bool> star::HotStorage::loaded_flags;
```




<hr>



### variable locations 

```C++
std::vector<StorageLocation> star::HotStorage::locations;
```




<hr>

------------------------------
The documentation for this class was generated from the following file `StarDS/include/stards.h`




# Struct star::KeyRegistry



[**ClassList**](annotated.md) **>** [**star**](namespacestar.md) **>** [**KeyRegistry**](structstar_1_1KeyRegistry.md)



_Global key registry using data-oriented design (Structure of Arrays)._ [More...](#detailed-description)

* `#include <stards.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  std::unordered\_map&lt; uint64\_t, uint16\_t &gt; | [**hash\_to\_index**](#variable-hash_to_index)  <br> |
|  std::vector&lt; uint64\_t &gt; | [**hashes**](#variable-hashes)  <br> |
|  std::unordered\_map&lt; std::string, uint16\_t &gt; | [**name\_to\_index**](#variable-name_to_index)  <br> |
|  std::vector&lt; std::string &gt; | [**names**](#variable-names)  <br> |
















## Public Functions

| Type | Name |
| ---: | :--- |
|  bool | [**contains**](#function-contains) (const std::string & key) const<br>_Check if key exists in registry._  |
|  uint16\_t | [**get\_index**](#function-get_index) (const std::string & key) const<br>_Get existing key index (throws if not found)._  |
|  uint16\_t | [**get\_or\_create**](#function-get_or_create) (const std::string & key) <br>_Get or create a key index._  |




























## Detailed Description


Stores all unique keys once with precomputed hashes for O(1) lookups. Keys are referenced by uint16 indices throughout the system. 


    
## Public Attributes Documentation




### variable hash\_to\_index 

```C++
std::unordered_map<uint64_t, uint16_t> star::KeyRegistry::hash_to_index;
```




<hr>



### variable hashes 

```C++
std::vector<uint64_t> star::KeyRegistry::hashes;
```




<hr>



### variable name\_to\_index 

```C++
std::unordered_map<std::string, uint16_t> star::KeyRegistry::name_to_index;
```




<hr>



### variable names 

```C++
std::vector<std::string> star::KeyRegistry::names;
```




<hr>
## Public Functions Documentation




### function contains 

_Check if key exists in registry._ 
```C++
inline bool star::KeyRegistry::contains (
    const std::string & key
) const
```




<hr>



### function get\_index 

_Get existing key index (throws if not found)._ 
```C++
inline uint16_t star::KeyRegistry::get_index (
    const std::string & key
) const
```





**Returns:**

uint16 index into the registry 





        

<hr>



### function get\_or\_create 

_Get or create a key index._ 
```C++
inline uint16_t star::KeyRegistry::get_or_create (
    const std::string & key
) 
```





**Returns:**

uint16 index into the registry 





        

<hr>

------------------------------
The documentation for this class was generated from the following file `StarDS/include/stards.h`


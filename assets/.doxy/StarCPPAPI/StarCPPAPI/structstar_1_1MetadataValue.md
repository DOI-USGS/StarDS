

# Struct star::MetadataValue



[**ClassList**](annotated.md) **>** [**star**](namespacestar.md) **>** [**MetadataValue**](structstar_1_1MetadataValue.md)



_Type-erased wrapper for metadata values._ [More...](#detailed-description)

* `#include <stards.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  ValueVariant | [**data**](#variable-data)  <br> |
|  DataType | [**dtype**](#variable-dtype)  <br> |
|  std::vector&lt; size\_t &gt; | [**shape**](#variable-shape)  <br> |
















## Public Functions

| Type | Name |
| ---: | :--- |
|  [**NDArray**](classstar_1_1NDArray.md)&lt; T &gt; | [**as**](#function-as) () const<br> |
|  bool | [**is\_array**](#function-is_array) () const<br> |
|  bool | [**is\_scalar**](#function-is_scalar) () const<br> |
|  size\_t | [**ndim**](#function-ndim) () const<br> |
|  size\_t | [**size**](#function-size) () const<br> |
|  std::shared\_ptr&lt; [**NDArray**](classstar_1_1NDArray.md)&lt; T &gt; &gt; | [**try\_as**](#function-try_as) () const<br> |
|  std::string | [**type\_name**](#function-type_name) () const<br> |




























## Detailed Description


Provides type introspection and safe casting for metadata retrieved without knowing the type ahead of time. Eliminates the need for type guessing loops. 


    
## Public Attributes Documentation




### variable data 

```C++
ValueVariant star::MetadataValue::data;
```




<hr>



### variable dtype 

```C++
DataType star::MetadataValue::dtype;
```




<hr>



### variable shape 

```C++
std::vector<size_t> star::MetadataValue::shape;
```




<hr>
## Public Functions Documentation




### function as 

```C++
template<typename T>
inline NDArray < T > star::MetadataValue::as () const
```




<hr>



### function is\_array 

```C++
inline bool star::MetadataValue::is_array () const
```




<hr>



### function is\_scalar 

```C++
inline bool star::MetadataValue::is_scalar () const
```




<hr>



### function ndim 

```C++
inline size_t star::MetadataValue::ndim () const
```




<hr>



### function size 

```C++
inline size_t star::MetadataValue::size () const
```




<hr>



### function try\_as 

```C++
template<typename T>
inline std::shared_ptr< NDArray < T > > star::MetadataValue::try_as () const
```




<hr>



### function type\_name 

```C++
inline std::string star::MetadataValue::type_name () const
```




<hr>

------------------------------
The documentation for this class was generated from the following file `StarDS/include/stards.h`


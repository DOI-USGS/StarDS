

# Struct star::SliceSpec



[**ClassList**](annotated.md) **>** [**star**](namespacestar.md) **>** [**SliceSpec**](structstar_1_1SliceSpec.md)



_Complete slice specification for n-dimensional array._ 

* `#include <stards.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  size\_t | [**element\_size**](#variable-element_size)  <br> |
|  bool | [**is\_contiguous**](#variable-is_contiguous)  <br> |
|  bool | [**is\_full\_rows**](#variable-is_full_rows)  <br> |
|  std::vector&lt; size\_t &gt; | [**output\_shape**](#variable-output_shape)  <br> |
|  std::vector&lt; [**Slice**](structstar_1_1Slice.md) &gt; | [**slices**](#variable-slices)  <br> |
|  size\_t | [**total\_elements**](#variable-total_elements)  <br> |












































## Public Attributes Documentation




### variable element\_size 

```C++
size_t star::SliceSpec::element_size;
```




<hr>



### variable is\_contiguous 

```C++
bool star::SliceSpec::is_contiguous;
```




<hr>



### variable is\_full\_rows 

```C++
bool star::SliceSpec::is_full_rows;
```




<hr>



### variable output\_shape 

```C++
std::vector<size_t> star::SliceSpec::output_shape;
```




<hr>



### variable slices 

```C++
std::vector<Slice> star::SliceSpec::slices;
```




<hr>



### variable total\_elements 

```C++
size_t star::SliceSpec::total_elements;
```




<hr>

------------------------------
The documentation for this class was generated from the following file `StarDS/include/stards.h`


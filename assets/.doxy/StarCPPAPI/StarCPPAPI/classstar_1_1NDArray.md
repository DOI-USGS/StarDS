

# Class star::NDArray

**template &lt;typename T&gt;**



[**ClassList**](annotated.md) **>** [**star**](namespacestar.md) **>** [**NDArray**](classstar_1_1NDArray.md)



_Modern n-dimensional array class with xtensor-style API._ [More...](#detailed-description)

* `#include <stards.h>`

















## Public Types

| Type | Name |
| ---: | :--- |
| typedef typename std::vector&lt; T &gt;::const\_iterator | [**const\_iterator**](#typedef-const_iterator)  <br> |
| typedef typename std::vector&lt; T &gt;::const\_reverse\_iterator | [**const\_reverse\_iterator**](#typedef-const_reverse_iterator)  <br> |
| typedef typename std::vector&lt; T &gt;::iterator | [**iterator**](#typedef-iterator)  <br> |
| typedef typename std::vector&lt; T &gt;::reverse\_iterator | [**reverse\_iterator**](#typedef-reverse_iterator)  <br> |
| typedef T | [**value\_type**](#typedef-value_type)  <br> |




















## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**NDArray**](#function-ndarray-16) () = default<br>_Default constructor._  |
|   | [**NDArray**](#function-ndarray-26) (const std::vector&lt; size\_t &gt; & shape\_in) <br>_Constructor with shape._  |
|   | [**NDArray**](#function-ndarray-36) (const std::vector&lt; size\_t &gt; & shape\_in, const T & initial\_value) <br>_Constructor with shape and initial value._  |
|   | [**NDArray**](#function-ndarray-46) (const std::vector&lt; T &gt; & data\_in, const std::vector&lt; size\_t &gt; & shape\_in) <br>_Constructor with data and shape._  |
|   | [**NDArray**](#function-ndarray-56) (std::vector&lt; T &gt; && data\_in, const std::vector&lt; size\_t &gt; & shape\_in) <br>_Move constructor with data and shape (zero-copy for Python bindings)._  |
|   | [**NDArray**](#function-ndarray-66) (const NDArray & other) <br>_Copy constructor._  |
|  T & | [**at**](#function-at-12) (const std::vector&lt; size\_t &gt; & indices) <br>_Get Array element._  |
|  const T & | [**at**](#function-at-22) (const std::vector&lt; size\_t &gt; & indices) const<br> |
|  iterator | [**begin**](#function-begin-12) () <br> |
|  const\_iterator | [**begin**](#function-begin-22) () const<br> |
|  const\_iterator | [**cbegin**](#function-cbegin) () const<br> |
|  const\_iterator | [**cend**](#function-cend) () const<br> |
|  std::vector&lt; T &gt; & | [**data**](#function-data-12) () <br>_Get reference to internal data vector._  |
|  const std::vector&lt; T &gt; & | [**data**](#function-data-22) () const<br> |
|  T \* | [**data\_ptr**](#function-data_ptr-12) () <br>_Get raw pointer to data._  |
|  const T \* | [**data\_ptr**](#function-data_ptr-22) () const<br> |
|  size\_t | [**dimension**](#function-dimension) () const<br>_Get number of dimensions._  |
|  iterator | [**end**](#function-end-12) () <br> |
|  const\_iterator | [**end**](#function-end-22) () const<br> |
|  T & | [**flat**](#function-flat-12) (size\_t index) <br>_Flat indexing for 1D access._  |
|  const T & | [**flat**](#function-flat-22) (size\_t index) const<br> |
|  T & | [**operator()**](#function-operator) (Indices... indices) <br>_Variadic operator() for multi-dimensional indexing._  |
|  const T & | [**operator()**](#function-operator_1) (Indices... indices) const<br> |
|  reverse\_iterator | [**rbegin**](#function-rbegin-12) () <br> |
|  const\_reverse\_iterator | [**rbegin**](#function-rbegin-22) () const<br> |
|  reverse\_iterator | [**rend**](#function-rend-12) () <br> |
|  const\_reverse\_iterator | [**rend**](#function-rend-22) () const<br> |
|  void | [**reshape**](#function-reshape) (const std::vector&lt; size\_t &gt; & new\_shape) <br>_Reshape array (no reallocation)._  |
|  void | [**resize**](#function-resize) (const std::vector&lt; size\_t &gt; & new\_shape, const T & fill\_value=T{}) <br>_Resize array (with reallocation)._  |
|  const std::vector&lt; size\_t &gt; & | [**shape**](#function-shape-12) () const<br>_Get shape vector._  |
|  size\_t | [**shape**](#function-shape-22) (size\_t dim) const<br>_Get specific dimension size._  |
|  size\_t | [**size**](#function-size) () const<br>_Get total number of elements._  |
|  const std::vector&lt; size\_t &gt; & | [**strides**](#function-strides) () const<br>_Get strides vector._  |


## Public Static Functions

| Type | Name |
| ---: | :--- |
|  std::enable\_if&lt; std::is\_arithmetic&lt; U &gt;::value, [**NDArray**](classstar_1_1NDArray.md#function-ndarray-16)&lt; T &gt; &gt;::type | [**arange**](#function-arange) (T start, T stop, T step=T{1}) <br>_Create range array (numeric types only)._  |
|  [**NDArray**](classstar_1_1NDArray.md#function-ndarray-16)&lt; T &gt; | [**empty**](#function-empty) (const std::vector&lt; size\_t &gt; & shape) <br>_Create uninitialized array._  |
|  [**NDArray**](classstar_1_1NDArray.md#function-ndarray-16) | [**from\_vector\_move**](#function-from_vector_move) (std::vector&lt; T &gt; & data\_in, const std::vector&lt; size\_t &gt; & shape\_in) <br>_Factory method to create_ [_**NDArray**_](classstar_1_1NDArray.md) _by moving vector (for Python bindings) SWIG doesn't handle rvalue references well, so we use this factory method._ |
|  [**NDArray**](classstar_1_1NDArray.md#function-ndarray-16)&lt; T &gt; | [**full**](#function-full) (const std::vector&lt; size\_t &gt; & shape, const T & value) <br>_Create array filled with specific value._  |
|  [**NDArray**](classstar_1_1NDArray.md#function-ndarray-16)&lt; T &gt; | [**ones**](#function-ones) (const std::vector&lt; size\_t &gt; & shape) <br>_Create one-initialized array._  |
|  [**NDArray**](classstar_1_1NDArray.md#function-ndarray-16)&lt; T &gt; | [**zeros**](#function-zeros) (const std::vector&lt; size\_t &gt; & shape) <br>_Create zero-initialized array._  |


























## Detailed Description


This class implements an n-dimensional array stored as a flat 1D vector with operator() indexing, iterators, and static factory methods. 


    
## Public Types Documentation




### typedef const\_iterator 

```C++
using star::NDArray< T >::const_iterator = typename std::vector<T>::const_iterator;
```




<hr>



### typedef const\_reverse\_iterator 

```C++
using star::NDArray< T >::const_reverse_iterator = typename std::vector<T>::const_reverse_iterator;
```




<hr>



### typedef iterator 

```C++
using star::NDArray< T >::iterator = typename std::vector<T>::iterator;
```




<hr>



### typedef reverse\_iterator 

```C++
using star::NDArray< T >::reverse_iterator = typename std::vector<T>::reverse_iterator;
```




<hr>



### typedef value\_type 

```C++
using star::NDArray< T >::value_type = T;
```




<hr>
## Public Functions Documentation




### function NDArray [1/6]

_Default constructor._ 
```C++
star::NDArray::NDArray () = default
```




<hr>



### function NDArray [2/6]

_Constructor with shape._ 
```C++
inline explicit star::NDArray::NDArray (
    const std::vector< size_t > & shape_in
) 
```





**Parameters:**


* `shape_in` Dimensions of the array 




        

<hr>



### function NDArray [3/6]

_Constructor with shape and initial value._ 
```C++
inline star::NDArray::NDArray (
    const std::vector< size_t > & shape_in,
    const T & initial_value
) 
```





**Parameters:**


* `shape_in` Dimensions of the array 
* `initial_value` Value to initialize all elements with 




        

<hr>



### function NDArray [4/6]

_Constructor with data and shape._ 
```C++
inline star::NDArray::NDArray (
    const std::vector< T > & data_in,
    const std::vector< size_t > & shape_in
) 
```





**Parameters:**


* `data_in` Flat data to use 
* `shape_in` Dimensions of the array 




        

<hr>



### function NDArray [5/6]

_Move constructor with data and shape (zero-copy for Python bindings)._ 
```C++
inline star::NDArray::NDArray (
    std::vector< T > && data_in,
    const std::vector< size_t > & shape_in
) 
```





**Parameters:**


* `data_in` Flat data to move from 
* `shape_in` Dimensions of the array 




        

<hr>



### function NDArray [6/6]

_Copy constructor._ 
```C++
inline star::NDArray::NDArray (
    const NDArray & other
) 
```





**Parameters:**


* `other` [**NDArray**](classstar_1_1NDArray.md) to copy from 




        

<hr>



### function at [1/2]

_Get Array element._ 
```C++
inline T & star::NDArray::at (
    const std::vector< size_t > & indices
) 
```





**Deprecated**

Use operator() instead 




        

<hr>



### function at [2/2]

```C++
inline const T & star::NDArray::at (
    const std::vector< size_t > & indices
) const
```




<hr>



### function begin [1/2]

```C++
inline iterator star::NDArray::begin () 
```




<hr>



### function begin [2/2]

```C++
inline const_iterator star::NDArray::begin () const
```




<hr>



### function cbegin 

```C++
inline const_iterator star::NDArray::cbegin () const
```




<hr>



### function cend 

```C++
inline const_iterator star::NDArray::cend () const
```




<hr>



### function data [1/2]

_Get reference to internal data vector._ 
```C++
inline std::vector< T > & star::NDArray::data () 
```




<hr>



### function data [2/2]

```C++
inline const std::vector< T > & star::NDArray::data () const
```




<hr>



### function data\_ptr [1/2]

_Get raw pointer to data._ 
```C++
inline T * star::NDArray::data_ptr () 
```




<hr>



### function data\_ptr [2/2]

```C++
inline const T * star::NDArray::data_ptr () const
```




<hr>



### function dimension 

_Get number of dimensions._ 
```C++
inline size_t star::NDArray::dimension () const
```




<hr>



### function end [1/2]

```C++
inline iterator star::NDArray::end () 
```




<hr>



### function end [2/2]

```C++
inline const_iterator star::NDArray::end () const
```




<hr>



### function flat [1/2]

_Flat indexing for 1D access._ 
```C++
inline T & star::NDArray::flat (
    size_t index
) 
```




<hr>



### function flat [2/2]

```C++
inline const T & star::NDArray::flat (
    size_t index
) const
```




<hr>



### function operator() 

_Variadic operator() for multi-dimensional indexing._ 
```C++
template<typename... Indices>
inline T & star::NDArray::operator() (
    Indices... indices
) 
```




<hr>



### function operator() 

```C++
template<typename... Indices>
inline const T & star::NDArray::operator() (
    Indices... indices
) const
```




<hr>



### function rbegin [1/2]

```C++
inline reverse_iterator star::NDArray::rbegin () 
```




<hr>



### function rbegin [2/2]

```C++
inline const_reverse_iterator star::NDArray::rbegin () const
```




<hr>



### function rend [1/2]

```C++
inline reverse_iterator star::NDArray::rend () 
```




<hr>



### function rend [2/2]

```C++
inline const_reverse_iterator star::NDArray::rend () const
```




<hr>



### function reshape 

_Reshape array (no reallocation)._ 
```C++
inline void star::NDArray::reshape (
    const std::vector< size_t > & new_shape
) 
```




<hr>



### function resize 

_Resize array (with reallocation)._ 
```C++
inline void star::NDArray::resize (
    const std::vector< size_t > & new_shape,
    const T & fill_value=T{}
) 
```




<hr>



### function shape [1/2]

_Get shape vector._ 
```C++
inline const std::vector< size_t > & star::NDArray::shape () const
```




<hr>



### function shape [2/2]

_Get specific dimension size._ 
```C++
inline size_t star::NDArray::shape (
    size_t dim
) const
```




<hr>



### function size 

_Get total number of elements._ 
```C++
inline size_t star::NDArray::size () const
```




<hr>



### function strides 

_Get strides vector._ 
```C++
inline const std::vector< size_t > & star::NDArray::strides () const
```




<hr>
## Public Static Functions Documentation




### function arange 

_Create range array (numeric types only)._ 
```C++
template<typename U>
static inline std::enable_if< std::is_arithmetic< U >::value, NDArray < T > >::type star::NDArray::arange (
    T start,
    T stop,
    T step=T{1}
) 
```




<hr>



### function empty 

_Create uninitialized array._ 
```C++
static inline NDArray < T > star::NDArray::empty (
    const std::vector< size_t > & shape
) 
```




<hr>



### function from\_vector\_move 

_Factory method to create_ [_**NDArray**_](classstar_1_1NDArray.md) _by moving vector (for Python bindings) SWIG doesn't handle rvalue references well, so we use this factory method._
```C++
static inline NDArray star::NDArray::from_vector_move (
    std::vector< T > & data_in,
    const std::vector< size_t > & shape_in
) 
```





**Parameters:**


* `data_in` Vector to move from 
* `shape_in` Dimensions of the array 




        

<hr>



### function full 

_Create array filled with specific value._ 
```C++
static inline NDArray < T > star::NDArray::full (
    const std::vector< size_t > & shape,
    const T & value
) 
```




<hr>



### function ones 

_Create one-initialized array._ 
```C++
static inline NDArray < T > star::NDArray::ones (
    const std::vector< size_t > & shape
) 
```




<hr>



### function zeros 

_Create zero-initialized array._ 
```C++
static inline NDArray < T > star::NDArray::zeros (
    const std::vector< size_t > & shape
) 
```




<hr>## Friends Documentation





### friend operator&lt;&lt; 

_Write array to output stream._ 
```C++
inline std::ostream & star::NDArray::operator<< (
    std::ostream & os,
    const NDArray < T > & arr
) 
```





**Parameters:**


* `os` Output stream 




        

<hr>



### friend operator&gt;&gt; 

_Read array from input stream._ 
```C++
inline std::istream & star::NDArray::operator>> (
    std::istream & is,
    NDArray < T > & arr
) 
```





**Parameters:**


* `is` Input stream 




        

<hr>

------------------------------
The documentation for this class was generated from the following file `StarDS/include/stards.h`


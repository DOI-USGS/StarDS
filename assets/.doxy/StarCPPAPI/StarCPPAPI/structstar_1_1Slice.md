

# Struct star::Slice



[**ClassList**](annotated.md) **>** [**star**](namespacestar.md) **>** [**Slice**](structstar_1_1Slice.md)



_Describes a slice along one dimension (Python-style slicing) Plain struct - no methods except helpers, just data._ [More...](#detailed-description)

* `#include <stards.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  size\_t | [**start**](#variable-start)  <br> |
|  size\_t | [**step**](#variable-step)   = `1`<br> |
|  size\_t | [**stop**](#variable-stop)  <br> |
















## Public Functions

| Type | Name |
| ---: | :--- |
|  size\_t | [**length**](#function-length) () const<br> |




























## Detailed Description


Usage: [**Slice**](structstar_1_1Slice.md){100, 200} // start=100, stop=200, step=1 (default) [**Slice**](structstar_1_1Slice.md){100, 200, 2} // start=100, stop=200, step=2 (every other) 


    
## Public Attributes Documentation




### variable start 

```C++
size_t star::Slice::start;
```




<hr>



### variable step 

```C++
size_t star::Slice::step;
```




<hr>



### variable stop 

```C++
size_t star::Slice::stop;
```




<hr>
## Public Functions Documentation




### function length 

```C++
inline size_t star::Slice::length () const
```




<hr>

------------------------------
The documentation for this class was generated from the following file `StarDS/include/stards.h`


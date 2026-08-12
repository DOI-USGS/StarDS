

# Class star::RangeReader



[**ClassList**](annotated.md) **>** [**star**](namespacestar.md) **>** [**RangeReader**](classstar_1_1RangeReader.md)










Inherited by the following classes: [star::HttpRangeReader](classstar_1_1HttpRangeReader.md),  [star::LocalRangeReader](classstar_1_1LocalRangeReader.md),  [star::MemoryRangeReader](classstar_1_1MemoryRangeReader.md),  [star::S3RangeReader](classstar_1_1S3RangeReader.md)
































## Public Functions

| Type | Name |
| ---: | :--- |
| virtual bool | [**ensure\_whole\_cached**](#function-ensure_whole_cached) (size\_t max\_bytes) <br> |
| virtual bool | [**good**](#function-good) () const = 0<br> |
| virtual size\_t | [**read\_at**](#function-read_at) (size\_t offset, size\_t len, std::vector&lt; char &gt; & out) = 0<br> |
| virtual size\_t | [**size\_or\_unknown**](#function-size_or_unknown) () = 0<br> |
| virtual  | [**~RangeReader**](#function-rangereader) () = default<br> |




























## Public Functions Documentation




### function ensure\_whole\_cached 

```C++
inline virtual bool star::RangeReader::ensure_whole_cached (
    size_t max_bytes
) 
```




<hr>



### function good 

```C++
virtual bool star::RangeReader::good () const = 0
```




<hr>



### function read\_at 

```C++
virtual size_t star::RangeReader::read_at (
    size_t offset,
    size_t len,
    std::vector< char > & out
) = 0
```




<hr>



### function size\_or\_unknown 

```C++
virtual size_t star::RangeReader::size_or_unknown () = 0
```




<hr>



### function ~RangeReader 

```C++
virtual star::RangeReader::~RangeReader () = default
```




<hr>

------------------------------
The documentation for this class was generated from the following file `StarDS/include/stards.h`


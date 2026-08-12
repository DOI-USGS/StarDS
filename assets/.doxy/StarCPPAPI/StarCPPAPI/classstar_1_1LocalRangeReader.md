

# Class star::LocalRangeReader



[**ClassList**](annotated.md) **>** [**star**](namespacestar.md) **>** [**LocalRangeReader**](classstar_1_1LocalRangeReader.md)








Inherits the following classes: [star::RangeReader](classstar_1_1RangeReader.md)






















































## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**LocalRangeReader**](#function-localrangereader) (const std::string & path) <br> |
| virtual bool | [**ensure\_whole\_cached**](#function-ensure_whole_cached) (size\_t max\_bytes) override<br> |
| virtual bool | [**good**](#function-good) () override const<br> |
| virtual size\_t | [**read\_at**](#function-read_at) (size\_t offset, size\_t len, std::vector&lt; char &gt; & out) override<br> |
| virtual size\_t | [**size\_or\_unknown**](#function-size_or_unknown) () override<br> |
|   | [**~LocalRangeReader**](#function-localrangereader) () override<br> |


## Public Functions inherited from star::RangeReader

See [star::RangeReader](classstar_1_1RangeReader.md)

| Type | Name |
| ---: | :--- |
| virtual bool | [**ensure\_whole\_cached**](classstar_1_1RangeReader.md#function-ensure_whole_cached) (size\_t max\_bytes) <br> |
| virtual bool | [**good**](classstar_1_1RangeReader.md#function-good) () const = 0<br> |
| virtual size\_t | [**read\_at**](classstar_1_1RangeReader.md#function-read_at) (size\_t offset, size\_t len, std::vector&lt; char &gt; & out) = 0<br> |
| virtual size\_t | [**size\_or\_unknown**](classstar_1_1RangeReader.md#function-size_or_unknown) () = 0<br> |
| virtual  | [**~RangeReader**](classstar_1_1RangeReader.md#function-rangereader) () = default<br> |






















































## Public Functions Documentation




### function LocalRangeReader 

```C++
inline explicit star::LocalRangeReader::LocalRangeReader (
    const std::string & path
) 
```




<hr>



### function ensure\_whole\_cached 

```C++
inline virtual bool star::LocalRangeReader::ensure_whole_cached (
    size_t max_bytes
) override
```



Implements [*star::RangeReader::ensure\_whole\_cached*](classstar_1_1RangeReader.md#function-ensure_whole_cached)


<hr>



### function good 

```C++
inline virtual bool star::LocalRangeReader::good () override const
```



Implements [*star::RangeReader::good*](classstar_1_1RangeReader.md#function-good)


<hr>



### function read\_at 

```C++
inline virtual size_t star::LocalRangeReader::read_at (
    size_t offset,
    size_t len,
    std::vector< char > & out
) override
```



Implements [*star::RangeReader::read\_at*](classstar_1_1RangeReader.md#function-read_at)


<hr>



### function size\_or\_unknown 

```C++
inline virtual size_t star::LocalRangeReader::size_or_unknown () override
```



Implements [*star::RangeReader::size\_or\_unknown*](classstar_1_1RangeReader.md#function-size_or_unknown)


<hr>



### function ~LocalRangeReader 

```C++
inline star::LocalRangeReader::~LocalRangeReader () override
```




<hr>

------------------------------
The documentation for this class was generated from the following file `StarDS/include/stards.h`


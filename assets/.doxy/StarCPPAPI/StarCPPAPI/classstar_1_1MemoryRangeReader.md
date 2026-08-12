

# Class star::MemoryRangeReader



[**ClassList**](annotated.md) **>** [**star**](namespacestar.md) **>** [**MemoryRangeReader**](classstar_1_1MemoryRangeReader.md)








Inherits the following classes: [star::RangeReader](classstar_1_1RangeReader.md)






















































## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**MemoryRangeReader**](#function-memoryrangereader) (std::vector&lt; char &gt; bytes) <br> |
| virtual bool | [**ensure\_whole\_cached**](#function-ensure_whole_cached) (size\_t) override<br> |
| virtual bool | [**good**](#function-good) () override const<br> |
| virtual size\_t | [**read\_at**](#function-read_at) (size\_t offset, size\_t len, std::vector&lt; char &gt; & out) override<br> |
| virtual size\_t | [**size\_or\_unknown**](#function-size_or_unknown) () override<br> |


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




### function MemoryRangeReader 

```C++
inline explicit star::MemoryRangeReader::MemoryRangeReader (
    std::vector< char > bytes
) 
```




<hr>



### function ensure\_whole\_cached 

```C++
inline virtual bool star::MemoryRangeReader::ensure_whole_cached (
    size_t
) override
```



Implements [*star::RangeReader::ensure\_whole\_cached*](classstar_1_1RangeReader.md#function-ensure_whole_cached)


<hr>



### function good 

```C++
inline virtual bool star::MemoryRangeReader::good () override const
```



Implements [*star::RangeReader::good*](classstar_1_1RangeReader.md#function-good)


<hr>



### function read\_at 

```C++
inline virtual size_t star::MemoryRangeReader::read_at (
    size_t offset,
    size_t len,
    std::vector< char > & out
) override
```



Implements [*star::RangeReader::read\_at*](classstar_1_1RangeReader.md#function-read_at)


<hr>



### function size\_or\_unknown 

```C++
inline virtual size_t star::MemoryRangeReader::size_or_unknown () override
```



Implements [*star::RangeReader::size\_or\_unknown*](classstar_1_1RangeReader.md#function-size_or_unknown)


<hr>

------------------------------
The documentation for this class was generated from the following file `StarDS/include/stards.h`


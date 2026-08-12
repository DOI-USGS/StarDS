

# Class star::S3RangeReader



[**ClassList**](annotated.md) **>** [**star**](namespacestar.md) **>** [**S3RangeReader**](classstar_1_1S3RangeReader.md)








Inherits the following classes: [star::RangeReader](classstar_1_1RangeReader.md)






















































## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**S3RangeReader**](#function-s3rangereader) (std::string bucket, std::string key, std::string region, [**S3Credentials**](structstar_1_1S3Credentials.md) creds) <br> |
| virtual bool | [**ensure\_whole\_cached**](#function-ensure_whole_cached) (size\_t max\_bytes) override<br> |
| virtual bool | [**good**](#function-good) () override const<br> |
| virtual size\_t | [**read\_at**](#function-read_at) (size\_t offset, size\_t len, std::vector&lt; char &gt; & out) override<br> |
| virtual size\_t | [**size\_or\_unknown**](#function-size_or_unknown) () override<br> |
|   | [**~S3RangeReader**](#function-s3rangereader) () override<br> |


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




### function S3RangeReader 

```C++
inline star::S3RangeReader::S3RangeReader (
    std::string bucket,
    std::string key,
    std::string region,
    S3Credentials creds
) 
```




<hr>



### function ensure\_whole\_cached 

```C++
inline virtual bool star::S3RangeReader::ensure_whole_cached (
    size_t max_bytes
) override
```



Implements [*star::RangeReader::ensure\_whole\_cached*](classstar_1_1RangeReader.md#function-ensure_whole_cached)


<hr>



### function good 

```C++
inline virtual bool star::S3RangeReader::good () override const
```



Implements [*star::RangeReader::good*](classstar_1_1RangeReader.md#function-good)


<hr>



### function read\_at 

```C++
inline virtual size_t star::S3RangeReader::read_at (
    size_t offset,
    size_t len,
    std::vector< char > & out
) override
```



Implements [*star::RangeReader::read\_at*](classstar_1_1RangeReader.md#function-read_at)


<hr>



### function size\_or\_unknown 

```C++
inline virtual size_t star::S3RangeReader::size_or_unknown () override
```



Implements [*star::RangeReader::size\_or\_unknown*](classstar_1_1RangeReader.md#function-size_or_unknown)


<hr>



### function ~S3RangeReader 

```C++
inline star::S3RangeReader::~S3RangeReader () override
```




<hr>

------------------------------
The documentation for this class was generated from the following file `StarDS/include/stards.h`


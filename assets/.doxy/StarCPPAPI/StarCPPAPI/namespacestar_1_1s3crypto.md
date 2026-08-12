

# Namespace star::s3crypto



[**Namespace List**](namespaces.md) **>** [**star**](namespacestar.md) **>** [**s3crypto**](namespacestar_1_1s3crypto.md)










































## Public Functions

| Type | Name |
| ---: | :--- |
|  std::array&lt; unsigned char, 32 &gt; | [**hmacSha256**](#function-hmacsha256) (const unsigned char \* key, size\_t key\_len, const unsigned char \* msg, size\_t msg\_len) <br> |
|  std::array&lt; unsigned char, 32 &gt; | [**hmacSha256**](#function-hmacsha256) (const std::string & key, const std::string & msg) <br> |
|  std::array&lt; unsigned char, 32 &gt; | [**sha256**](#function-sha256) (const unsigned char \* data, size\_t len) <br> |
|  std::array&lt; unsigned char, 32 &gt; | [**sha256**](#function-sha256) (const std::string & data) <br> |




























## Public Functions Documentation




### function hmacSha256 

```C++
inline std::array< unsigned char, 32 > star::s3crypto::hmacSha256 (
    const unsigned char * key,
    size_t key_len,
    const unsigned char * msg,
    size_t msg_len
) 
```




<hr>



### function hmacSha256 

```C++
inline std::array< unsigned char, 32 > star::s3crypto::hmacSha256 (
    const std::string & key,
    const std::string & msg
) 
```




<hr>



### function sha256 

```C++
inline std::array< unsigned char, 32 > star::s3crypto::sha256 (
    const unsigned char * data,
    size_t len
) 
```




<hr>



### function sha256 

```C++
inline std::array< unsigned char, 32 > star::s3crypto::sha256 (
    const std::string & data
) 
```




<hr>

------------------------------
The documentation for this class was generated from the following file `StarDS/include/stards.h`


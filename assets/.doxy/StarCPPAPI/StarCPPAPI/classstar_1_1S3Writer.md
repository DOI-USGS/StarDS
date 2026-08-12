

# Class star::S3Writer



[**ClassList**](annotated.md) **>** [**star**](namespacestar.md) **>** [**S3Writer**](classstar_1_1S3Writer.md)



_S3 writer for uploading objects._ [More...](#detailed-description)

* `#include <stards.h>`















## Classes

| Type | Name |
| ---: | :--- |
| struct | [**HeaderData**](structstar_1_1S3Writer_1_1HeaderData.md) <br> |






















## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**S3Writer**](#function-s3writer) (const std::string & bucket, const std::string & key, const std::string & region, const [**S3Credentials**](structstar_1_1S3Credentials.md) & creds) <br> |
|  void | [**putObject**](#function-putobject) (const char \* data, size\_t size) <br>_Upload object to S3._  |
|   | [**~S3Writer**](#function-s3writer) () <br> |


## Public Static Functions

| Type | Name |
| ---: | :--- |
|  size\_t | [**HeaderCallback**](#function-headercallback) (char \* buffer, size\_t size, size\_t nitems, void \* userdata) <br> |
|  size\_t | [**WriteCallback**](#function-writecallback) (char \* ptr, size\_t size, size\_t nmemb, void \* userdata) <br> |


























## Detailed Description


Handles uploading data to S3 using PUT requests with AWS Signature V4 authentication. Supports single-shot uploads (entire object in one PUT request). 


    
## Public Functions Documentation




### function S3Writer 

```C++
inline star::S3Writer::S3Writer (
    const std::string & bucket,
    const std::string & key,
    const std::string & region,
    const S3Credentials & creds
) 
```




<hr>



### function putObject 

_Upload object to S3._ 
```C++
inline void star::S3Writer::putObject (
    const char * data,
    size_t size
) 
```





**Parameters:**


* `data` Pointer to data to upload 
* `size` Size of data in bytes 




        

<hr>



### function ~S3Writer 

```C++
inline star::S3Writer::~S3Writer () 
```




<hr>
## Public Static Functions Documentation




### function HeaderCallback 

```C++
static inline size_t star::S3Writer::HeaderCallback (
    char * buffer,
    size_t size,
    size_t nitems,
    void * userdata
) 
```




<hr>



### function WriteCallback 

```C++
static inline size_t star::S3Writer::WriteCallback (
    char * ptr,
    size_t size,
    size_t nmemb,
    void * userdata
) 
```




<hr>

------------------------------
The documentation for this class was generated from the following file `StarDS/include/stards.h`


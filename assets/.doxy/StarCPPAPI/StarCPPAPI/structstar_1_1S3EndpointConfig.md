

# Struct star::S3EndpointConfig



[**ClassList**](annotated.md) **>** [**star**](namespacestar.md) **>** [**S3EndpointConfig**](structstar_1_1S3EndpointConfig.md)



_S3 endpoint resolution (default AWS, or an override for S3-compatible services such as MinIO and for local testing)._ [More...](#detailed-description)

* `#include <stards.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  std::string | [**endpoint**](#variable-endpoint)  <br> |
|  bool | [**path\_style**](#variable-path_style)   = `false`<br> |
|  bool | [**use\_https**](#variable-use_https)   = `true`<br> |
















## Public Functions

| Type | Name |
| ---: | :--- |
|  std::string | [**canonical\_uri**](#function-canonical_uri) (const std::string & bucket, const std::string & encoded\_key) const<br> |
|  std::string | [**host**](#function-host) (const std::string & bucket, const std::string & region) const<br> |
|  std::string | [**url**](#function-url) (const std::string & bucket, const std::string & region, const std::string & encoded\_key) const<br> |


## Public Static Functions

| Type | Name |
| ---: | :--- |
|  bool | [**env\_is\_false**](#function-env_is_false) (const char \* name) <br> |
|  [**S3EndpointConfig**](structstar_1_1S3EndpointConfig.md) | [**resolve**](#function-resolve) () <br> |


























## Detailed Description


Controlled by environment variables (GDAL-compatible names):
* AWS\_S3\_ENDPOINT host[:port] to use instead of s3.&lt;region&gt;.amazonaws.com
* AWS\_VIRTUAL\_HOSTING "FALSE"/"NO" -&gt; path-style (endpoint/bucket/key); otherwise virtual-hosted (bucket.endpoint/key)
* AWS\_HTTPS "NO"/"FALSE" -&gt; [http://](http://) scheme (default [https://](https://))




With none set, behavior is byte-identical to the historical "https://&lt;bucket&gt;.s3.&lt;region&gt;.amazonaws.com/&lt;key&gt;" (virtual-hosted, https).


IMPORTANT: the SigV4 `host` header must equal the actual connection host (including any :port) AND the signed canonical URI must equal the URL path, so host(), url(), and canonical\_uri() are all derived here to stay consistent. 


    
## Public Attributes Documentation




### variable endpoint 

```C++
std::string star::S3EndpointConfig::endpoint;
```




<hr>



### variable path\_style 

```C++
bool star::S3EndpointConfig::path_style;
```




<hr>



### variable use\_https 

```C++
bool star::S3EndpointConfig::use_https;
```




<hr>
## Public Functions Documentation




### function canonical\_uri 

```C++
inline std::string star::S3EndpointConfig::canonical_uri (
    const std::string & bucket,
    const std::string & encoded_key
) const
```




<hr>



### function host 

```C++
inline std::string star::S3EndpointConfig::host (
    const std::string & bucket,
    const std::string & region
) const
```




<hr>



### function url 

```C++
inline std::string star::S3EndpointConfig::url (
    const std::string & bucket,
    const std::string & region,
    const std::string & encoded_key
) const
```




<hr>
## Public Static Functions Documentation




### function env\_is\_false 

```C++
static inline bool star::S3EndpointConfig::env_is_false (
    const char * name
) 
```




<hr>



### function resolve 

```C++
static inline S3EndpointConfig star::S3EndpointConfig::resolve () 
```




<hr>

------------------------------
The documentation for this class was generated from the following file `StarDS/include/stards.h`


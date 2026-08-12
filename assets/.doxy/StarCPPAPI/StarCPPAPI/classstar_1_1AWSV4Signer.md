

# Class star::AWSV4Signer



[**ClassList**](annotated.md) **>** [**star**](namespacestar.md) **>** [**AWSV4Signer**](classstar_1_1AWSV4Signer.md)



_AWS Signature Version 4 signer._ [More...](#detailed-description)

* `#include <stards.h>`





































## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**AWSV4Signer**](#function-awsv4signer) (const std::string & access\_key, const std::string & secret\_key, const std::string & region, const std::string & session\_token="") <br> |
|  const std::string & | [**getSessionToken**](#function-getsessiontoken) () const<br> |
|  std::string | [**signRequest**](#function-signrequest) (const std::string & method, const std::string & bucket, const std::string & key, const std::string & content\_hash, const std::map&lt; std::string, std::string &gt; & headers) const<br>_Sign an S3 request using AWS Signature Version 4._  |


## Public Static Functions

| Type | Name |
| ---: | :--- |
|  std::string | [**hexEncode**](#function-hexencode) (const unsigned char \* data, size\_t len) <br>_Convert binary data to hex string (public utility)._  |
|  std::string | [**urlEncode**](#function-urlencode) (const std::string & value) <br> |


























## Detailed Description


Implements AWS SigV4 authentication for S3 requests. Uses OpenSSL for the SHA-256 / HMAC-SHA256 primitives on native builds and mbedTLS under WebAssembly (see the s3crypto shim above). This provides authentication without requiring the full AWS SDK. 


    
## Public Functions Documentation




### function AWSV4Signer 

```C++
inline star::AWSV4Signer::AWSV4Signer (
    const std::string & access_key,
    const std::string & secret_key,
    const std::string & region,
    const std::string & session_token=""
) 
```




<hr>



### function getSessionToken 

```C++
inline const std::string & star::AWSV4Signer::getSessionToken () const
```




<hr>



### function signRequest 

_Sign an S3 request using AWS Signature Version 4._ 
```C++
inline std::string star::AWSV4Signer::signRequest (
    const std::string & method,
    const std::string & bucket,
    const std::string & key,
    const std::string & content_hash,
    const std::map< std::string, std::string > & headers
) const
```





**Parameters:**


* `method` HTTP method (GET, PUT, etc.) 
* `bucket` S3 bucket name 
* `key` S3 object key 
* `content_hash` SHA256 hash of request body (hex-encoded) 
* `headers` Request headers (must include host and x-amz-date) 



**Returns:**

Authorization header value 





        

<hr>
## Public Static Functions Documentation




### function hexEncode 

_Convert binary data to hex string (public utility)._ 
```C++
static inline std::string star::AWSV4Signer::hexEncode (
    const unsigned char * data,
    size_t len
) 
```




<hr>



### function urlEncode 

```C++
static inline std::string star::AWSV4Signer::urlEncode (
    const std::string & value
) 
```




<hr>

------------------------------
The documentation for this class was generated from the following file `StarDS/include/stards.h`


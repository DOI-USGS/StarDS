

# Struct star::S3Credentials



[**ClassList**](annotated.md) **>** [**star**](namespacestar.md) **>** [**S3Credentials**](structstar_1_1S3Credentials.md)



_AWS Credentials with resolution chain._ [More...](#detailed-description)

* `#include <stards.h>`





















## Public Attributes

| Type | Name |
| ---: | :--- |
|  std::string | [**access\_key**](#variable-access_key)  <br> |
|  std::string | [**secret\_key**](#variable-secret_key)  <br> |
|  std::string | [**session\_token**](#variable-session_token)  <br> |


















## Public Static Functions

| Type | Name |
| ---: | :--- |
|  [**S3Credentials**](structstar_1_1S3Credentials.md) | [**resolve**](#function-resolve) (const std::string & profile="") <br>_Resolve credentials using standard AWS chain._  |


























## Detailed Description


Resolves credentials in standard AWS order:
* Environment variables
* AWS SSO session
* Credentials file 




    
## Public Attributes Documentation




### variable access\_key 

```C++
std::string star::S3Credentials::access_key;
```




<hr>



### variable secret\_key 

```C++
std::string star::S3Credentials::secret_key;
```




<hr>



### variable session\_token 

```C++
std::string star::S3Credentials::session_token;
```




<hr>
## Public Static Functions Documentation




### function resolve 

_Resolve credentials using standard AWS chain._ 
```C++
static inline S3Credentials star::S3Credentials::resolve (
    const std::string & profile=""
) 
```





**Parameters:**


* `profile` Profile name (from AWS\_PROFILE env var or "default") 



**Returns:**

[**S3Credentials**](structstar_1_1S3Credentials.md) 




**Exception:**


* `std::runtime_error` if no credentials found 




        

<hr>

------------------------------
The documentation for this class was generated from the following file `StarDS/include/stards.h`


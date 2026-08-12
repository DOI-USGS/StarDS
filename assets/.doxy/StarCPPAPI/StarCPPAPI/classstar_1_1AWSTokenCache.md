

# Class star::AWSTokenCache



[**ClassList**](annotated.md) **>** [**star**](namespacestar.md) **>** [**AWSTokenCache**](classstar_1_1AWSTokenCache.md)



_AWS SSO Token Cache reader._ [More...](#detailed-description)

* `#include <stards.h>`







































## Public Static Functions

| Type | Name |
| ---: | :--- |
|  std::optional&lt; std::tuple&lt; std::string, std::string, std::string &gt; &gt; | [**getCredentialsFromToken**](#function-getcredentialsfromtoken) (const std::string & access\_token, const std::string & account\_id, const std::string & role\_name, const std::string & region) <br>_Get temporary credentials using SSO token._  |
|  std::optional&lt; SSOToken &gt; | [**readToken**](#function-readtoken) (const std::string & start\_url, const std::string & sso\_session="") <br>_Find and read SSO token for a given start URL._  |


























## Detailed Description


Reads and manages SSO tokens cached by AWS CLI 


    
## Public Static Functions Documentation




### function getCredentialsFromToken 

_Get temporary credentials using SSO token._ 
```C++
static inline std::optional< std::tuple< std::string, std::string, std::string > > star::AWSTokenCache::getCredentialsFromToken (
    const std::string & access_token,
    const std::string & account_id,
    const std::string & role_name,
    const std::string & region
) 
```



Makes API call to AWS SSO service to exchange token for credentials




**Parameters:**


* `access_token` SSO access token 
* `account_id` AWS account ID 
* `role_name` IAM role name 
* `region` AWS region 



**Returns:**

tuple of (access\_key, secret\_key, session\_token) 





        

<hr>



### function readToken 

_Find and read SSO token for a given start URL._ 
```C++
static inline std::optional< SSOToken > star::AWSTokenCache::readToken (
    const std::string & start_url,
    const std::string & sso_session=""
) 
```





**Parameters:**


* `start_url` The SSO start URL from AWS config 
* `sso_session` The sso-session name (if the profile uses one). When non-empty it is the cache-key the AWS CLI hashes; otherwise the start\_url is used. Defaults to "" for compatibility. 



**Returns:**

SSOToken if found and valid, empty optional otherwise 





        

<hr>

------------------------------
The documentation for this class was generated from the following file `StarDS/include/stards.h`


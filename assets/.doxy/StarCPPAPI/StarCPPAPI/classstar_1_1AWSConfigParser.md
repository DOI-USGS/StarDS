

# Class star::AWSConfigParser



[**ClassList**](annotated.md) **>** [**star**](namespacestar.md) **>** [**AWSConfigParser**](classstar_1_1AWSConfigParser.md)



_AWS Configuration file parser._ [More...](#detailed-description)

* `#include <stards.h>`





































## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**AWSConfigParser**](#function-awsconfigparser) (const std::string & config\_file) <br> |
|  std::string | [**getValue**](#function-getvalue) (const std::string & profile, const std::string & key, const std::string & default\_value="") const<br> |
|  bool | [**hasProfile**](#function-hasprofile) (const std::string & profile) const<br> |


## Public Static Functions

| Type | Name |
| ---: | :--- |
|  std::string | [**getConfigPath**](#function-getconfigpath) () <br> |
|  std::string | [**getCredentialsPath**](#function-getcredentialspath) () <br> |


























## Detailed Description


Parses INI-style configuration files like ~/.aws/config and ~/.aws/credentials 


    
## Public Functions Documentation




### function AWSConfigParser 

```C++
inline star::AWSConfigParser::AWSConfigParser (
    const std::string & config_file
) 
```




<hr>



### function getValue 

```C++
inline std::string star::AWSConfigParser::getValue (
    const std::string & profile,
    const std::string & key,
    const std::string & default_value=""
) const
```




<hr>



### function hasProfile 

```C++
inline bool star::AWSConfigParser::hasProfile (
    const std::string & profile
) const
```




<hr>
## Public Static Functions Documentation




### function getConfigPath 

```C++
static inline std::string star::AWSConfigParser::getConfigPath () 
```




<hr>



### function getCredentialsPath 

```C++
static inline std::string star::AWSConfigParser::getCredentialsPath () 
```




<hr>

------------------------------
The documentation for this class was generated from the following file `StarDS/include/stards.h`


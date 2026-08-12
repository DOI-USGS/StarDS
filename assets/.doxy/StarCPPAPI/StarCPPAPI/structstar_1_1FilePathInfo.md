

# Struct star::FilePathInfo



[**ClassList**](annotated.md) **>** [**star**](namespacestar.md) **>** [**FilePathInfo**](structstar_1_1FilePathInfo.md)






















## Public Types

| Type | Name |
| ---: | :--- |
| enum  | [**Type**](#enum-type)  <br> |




## Public Attributes

| Type | Name |
| ---: | :--- |
|  std::string | [**bucket**](#variable-bucket)  <br> |
|  std::string | [**key**](#variable-key)  <br> |
|  std::string | [**path**](#variable-path)  <br> |
|  std::string | [**region**](#variable-region)  <br> |
|  Type | [**type**](#variable-type)  <br> |












































## Public Types Documentation




### enum Type 

```C++
enum star::FilePathInfo::Type {
    LOCAL,
    HTTP,
    S3,
    MEMORY
};
```




<hr>
## Public Attributes Documentation




### variable bucket 

```C++
std::string star::FilePathInfo::bucket;
```




<hr>



### variable key 

```C++
std::string star::FilePathInfo::key;
```




<hr>



### variable path 

```C++
std::string star::FilePathInfo::path;
```




<hr>



### variable region 

```C++
std::string star::FilePathInfo::region;
```




<hr>



### variable type 

```C++
Type star::FilePathInfo::type;
```




<hr>

------------------------------
The documentation for this class was generated from the following file `StarDS/include/stards.h`


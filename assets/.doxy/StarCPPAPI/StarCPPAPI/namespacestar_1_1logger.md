

# Namespace star::logger



[**Namespace List**](namespaces.md) **>** [**star**](namespacestar.md) **>** [**logger**](namespacestar_1_1logger.md)






















## Public Types

| Type | Name |
| ---: | :--- |
| enum  | [**LogLevel**](#enum-loglevel)  <br> |






## Public Static Attributes

| Type | Name |
| ---: | :--- |
|  const char \* | [**LOG\_LEVEL\_STRINGS**](#variable-log_level_strings)   = `{"TRACE", "DEBUG", "INFO", "WARN", "ERROR"}`<br> |
|  LogLevel | [**current\_log\_level**](#variable-current_log_level)   = `STARDS\_ERROR`<br> |














## Public Functions

| Type | Name |
| ---: | :--- |
|  LogLevel | [**get\_log\_level**](#function-get_log_level) () <br>_Gets the current log level._  |
|  void | [**log\_impl**](#function-log_impl) (std::ostream & os, const T & first) <br>_Base implementation for logging a single value._  |
|  void | [**log\_impl**](#function-log_impl) (std::ostream & os, const T & first, const Args &... args) <br>_Recursive implementation for logging multiple values._  |
|  void | [**log\_internal**](#function-log_internal) (LogLevel level, int line, const char \* func, const Args &... args) <br>_Internal logging function that formats and outputs log messages._  |
|  void | [**set\_log\_level**](#function-set_log_level) (LogLevel level) <br>_Sets the current log level._  |




























## Public Types Documentation




### enum LogLevel 

```C++
enum star::logger::LogLevel {
    STARDS_TRACE =0,
    STARDS_DEBUG,
    STARDS_INFO,
    STARDS_WARN,
    STARDS_ERROR
};
```




<hr>
## Public Static Attributes Documentation




### variable LOG\_LEVEL\_STRINGS 

```C++
const char* star::logger::LOG_LEVEL_STRINGS[];
```




<hr>



### variable current\_log\_level 

```C++
LogLevel star::logger::current_log_level;
```




<hr>
## Public Functions Documentation




### function get\_log\_level 

_Gets the current log level._ 
```C++
inline LogLevel star::logger::get_log_level () 
```





**Returns:**

Current log level 





        

<hr>



### function log\_impl 

_Base implementation for logging a single value._ 
```C++
template<typename T>
void star::logger::log_impl (
    std::ostream & os,
    const T & first
) 
```





**Parameters:**


* `os` Output stream to write to 
* `first` Value to log 




        

<hr>



### function log\_impl 

_Recursive implementation for logging multiple values._ 
```C++
template<typename T, typename... Args>
void star::logger::log_impl (
    std::ostream & os,
    const T & first,
    const Args &... args
) 
```





**Parameters:**


* `os` Output stream to write to 
* `first` First value to log 
* `args` Remaining values to log 




        

<hr>



### function log\_internal 

_Internal logging function that formats and outputs log messages._ 
```C++
template<typename... Args>
void star::logger::log_internal (
    LogLevel level,
    int line,
    const char * func,
    const Args &... args
) 
```





**Parameters:**


* `level` Log level of the message 
* `line` Source code line number 
* `func` Function name 
* `args` Values to log 




        

<hr>



### function set\_log\_level 

_Sets the current log level._ 
```C++
inline void star::logger::set_log_level (
    LogLevel level
) 
```





**Parameters:**


* `level` New log level to use 




        

<hr>

------------------------------
The documentation for this class was generated from the following file `StarDS/include/stards.h`




# Class star::ThreadPool



[**ClassList**](annotated.md) **>** [**star**](namespacestar.md) **>** [**ThreadPool**](classstar_1_1ThreadPool.md)



_Simple thread pool for parallel block operations._ [More...](#detailed-description)

* `#include <stards.h>`





































## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**ThreadPool**](#function-threadpool-13) (size\_t num\_threads=0) <br>_Construct thread pool._  |
|   | [**ThreadPool**](#function-threadpool-23) (const ThreadPool &) = delete<br> |
|   | [**ThreadPool**](#function-threadpool-33) (ThreadPool &&) = delete<br> |
|  std::future&lt; void &gt; | [**enqueue**](#function-enqueue) (Func && f) <br>_Enqueue a task for execution._  |
|  [**ThreadPool**](classstar_1_1ThreadPool.md#function-threadpool-13) & | [**operator=**](#function-operator) (const [**ThreadPool**](classstar_1_1ThreadPool.md#function-threadpool-13) &) = delete<br> |
|  [**ThreadPool**](classstar_1_1ThreadPool.md#function-threadpool-13) & | [**operator=**](#function-operator_1) ([**ThreadPool**](classstar_1_1ThreadPool.md#function-threadpool-13) &&) = delete<br> |
|  void | [**parallel\_for**](#function-parallel_for) (size\_t start, size\_t end, Func && func) <br>_Execute function in parallel for range [start, end)._  |
|  size\_t | [**size**](#function-size) () const<br>_Get number of worker threads._  |
|   | [**~ThreadPool**](#function-threadpool) () <br>_Destructor - stops all worker threads._  |




























## Detailed Description


Provides work-stealing task execution for parallel compression/decompression. Thread-safe and exception-safe. 


    
## Public Functions Documentation




### function ThreadPool [1/3]

_Construct thread pool._ 
```C++
inline explicit star::ThreadPool::ThreadPool (
    size_t num_threads=0
) 
```





**Parameters:**


* `num_threads` Number of worker threads (0 = auto-detect) 




        

<hr>



### function ThreadPool [2/3]

```C++
star::ThreadPool::ThreadPool (
    const ThreadPool &
) = delete
```




<hr>



### function ThreadPool [3/3]

```C++
star::ThreadPool::ThreadPool (
    ThreadPool &&
) = delete
```




<hr>



### function enqueue 

_Enqueue a task for execution._ 
```C++
template<typename Func>
inline std::future< void > star::ThreadPool::enqueue (
    Func && f
) 
```





**Parameters:**


* `f` Function to execute 



**Returns:**

Future for result 





        

<hr>



### function operator= 

```C++
ThreadPool & star::ThreadPool::operator= (
    const ThreadPool &
) = delete
```




<hr>



### function operator= 

```C++
ThreadPool & star::ThreadPool::operator= (
    ThreadPool &&
) = delete
```




<hr>



### function parallel\_for 

_Execute function in parallel for range [start, end)._ 
```C++
template<typename Func>
inline void star::ThreadPool::parallel_for (
    size_t start,
    size_t end,
    Func && func
) 
```





**Parameters:**


* `start` Start index (inclusive) 
* `end` End index (exclusive) 
* `func` Function to execute for each index 




        

<hr>



### function size 

_Get number of worker threads._ 
```C++
inline size_t star::ThreadPool::size () const
```




<hr>



### function ~ThreadPool 

_Destructor - stops all worker threads._ 
```C++
inline star::ThreadPool::~ThreadPool () 
```




<hr>

------------------------------
The documentation for this class was generated from the following file `StarDS/include/stards.h`


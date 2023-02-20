# Parallelism for cheap
## But still basically free


### Review

The Standard Template Library contains several algorithms that work out of the box for standard containers, as well as our own that we implement the appropriate iterator for.  These algorithms can also execute in parallel as long as the machine running the program supports it.  To start, we'll go through an example of how this works.

To simulate processing a value, we'll define a function that has no other purpose than to take some time and be sure that the compiler can't optimize it away.

```cpp
#include <random>

using u32 = unsigned;


int do_work(int n)
{
    constexpr u32 AMOUNT_OF_WORK = 100;

    std::random_device r;
    std::default_random_engine eng(r());
    std::uniform_int_distribution<int> uniform_dist(10, 1000);

    int sum = n;

    for (u32 i = 0; i < AMOUNT_OF_WORK; ++i)
    {
        sum += uniform_dist(eng) % 3;
    }

    return sum;
}
```

We'll use the function by applying it to the elements of a vector and storing the results in another vector.  The first implementation does this using a for loop.

```cpp
#include <cassert>

using intVec = std::vector<int>;


void process_vector_for(intVec& src, intVec& dst)
{    
    assert(dst.size() == src.size());

    auto n_elements = src.size();

    for (u32 i = 0; i < n_elements; ++i)
    {
        dst[i] = do_work(src[i]);
    }
}
```

This kind of operation is called a transform and the C++ standard library has an algorithm for that.  

```cpp
#include <algorithm>


void process_vector_stl(intVec& src, intVec& dst)
{
    assert(dst.size() == src.size());
    
    std::transform(src.begin(), src.end(), dst.begin(), do_work);
}
```

Parallelism is free so we'll use it to compare processing times with the other two.

```cpp
#include <execution>


void process_vector_par(intVec& src, intVec& dst)
{
    assert(dst.size() == src.size());

    std::transform(std::execution::par, src.begin(), src.end(), dst.begin(), do_work);
}
```

Here is a main function to compare the processing times of each function.

```cpp
#include "stopwatch.hpp"
// https://github.com/adam-lafontaine/Cpp_Utilities/blob/master/stopwatch/stopwatch.hpp


int main()
{
    u32 n_elements = 1'000'000;

    Stopwatch sw;
    auto const print_time = [&sw]() { printf("%9.3f ms\n", sw.get_time_milli()); };

    printf("creating vectors:     ");
    sw.start();

    intVec src(n_elements, 0);
    intVec dst(n_elements, 0);
    print_time();

    printf("process_vector_for(): ");
    sw.start();

    process_vector_for(src, dst);
    print_time();

    printf("process_vector_stl(): ");
    sw.start();

    process_vector_stl(src, dst);
    print_time();

    printf("process_vector_par(): ");
    sw.start();

    process_vector_par(src, dst);
    print_time();
}
```

The program gives the following output.

```plaintext
creating vectors:         1.530 ms
process_vector_for():  2377.826 ms
process_vector_stl():  2371.840 ms
process_vector_par():   487.976 ms
```

As expected the raw for loop and `std::transform` are the same, while the parallel version is much faster.

### Cheap parallelism

Performing the same function on multiple elements in parallel is convinent and a big time saver.  Another use for multi-threading is allowing separate functions to run in parallel.  Fortunately this is also pretty easy.  Consider the following functions.

```cpp
void print_message1()
{
    printf("Hello "); printf("my "); printf("name "); printf("is "); printf("Adam ");
}


void print_message2()
{
    printf("A1 "); printf("A2 "); printf("A3 "); printf("A4 "); printf("A5 ");
}


void print_message3()
{
    printf("B1 "); printf("B2 "); printf("B3 "); printf("B4 "); printf("B5 ");
}


void print_message4()
{
    printf("C1 "); printf("C2 "); printf("C3 "); printf("C4 "); printf("C5 ");
}


void print_message5()
{
    printf("D1 "); printf("D2 "); printf("D3 "); printf("D4 "); printf("D5 ");
}
```

We can execute them in parallel using the same principle as earlier.  This time we make a vector (or array) of functions and call `std::for_each` with a lambda that only calls each function.

```cpp
#include <functional>


void print_messages_par()
{
    std::vector<std::function<void()>> funcs
    {
        print_message1,
        print_message2,
        print_message3,
        print_message4,
        print_message5
    };

    auto const execute = [](auto const& func) { func(); };

    std::for_each(std::execution::par, funcs.begin(), funcs.end(), execute);
}
```

When executed, the print statements are displayed in no particular order.

```plaintext
Hello my A1 A2 B1 B2 C1 C2 D1 C3 C4 name is Adam D2 D3 D4 D5 C5 A3 A4 A5 B3 B4 B5
```

This could have been accomplished using a vector of function pointers instead, but `std::function` allows us to use anything that is callable.  For instance, lambdas will also work.

```cpp

void print_messages_par()
{
    std::vector<std::function<void()>> funcs
    {
        []() { printf("Hello "); printf("my "); printf("name "); printf("is "); printf("Adam "); },
        []() { printf("A1 "); printf("A2 "); printf("A3 "); printf("A4 "); printf("A5 "); },
        []() { printf("B1 "); printf("B2 "); printf("B3 "); printf("B4 "); printf("B5 "); },
        []() { printf("C1 "); printf("C2 "); printf("C3 "); printf("C4 "); printf("C5 "); },
        []() { printf("D1 "); printf("D2 "); printf("D3 "); printf("D4 "); printf("D5 "); },
    };

    auto const execute = [](auto const& func) { func(); };

    std::for_each(std::execution::par, funcs.begin(), funcs.end(), execute);
}
```

A combination of functions and lamdas work as well.

```cpp

void print_messages_par()
{
    std::vector<std::function<void()>> funcs
    {
        []() { printf("Hello "); printf("my "); printf("name "); printf("is "); printf("Adam "); },
        []() { printf("A1 "); printf("A2 "); printf("A3 "); printf("A4 "); printf("A5 "); },
        print_message3,
        print_message4,
        print_message5
    };

    auto const execute = [](auto const& func) { func(); };

    std::for_each(std::execution::par, funcs.begin(), funcs.end(), execute);
}
```

### Example

For demonstration purposes, we can wrap the call to `std::for_each` in a function for parallel execution and in another for sequential.


```cpp
using f_list = std::vector<std::function<void()>>;


void execute_par(f_list const& funcs)
{
    std::for_each(std::execution::par, funcs.begin(), funcs.end(), [](auto const& func) { func(); });
}


void execute_seq(f_list const& funcs)
{
    std::for_each(funcs.begin(), funcs.end(), [](auto const& func) { func(); });
}
```

Our `process_vector` functions take source and destination vectors as parameters, so we cannot simply use them as is in our vector of functions.  We need to construct a lambda for each one that calls it with the appropriate source and destination.

```cpp
f_list funcs
{
    [&]() { process_vector_for(src, dst1); },
    [&]() { process_vector_stl(src, dst2); },
    [&]() { process_vector_par(src, dst3); },
};
```

Here is a program to demonstrate.

```cpp
int main()
{
    u32 n_elements = 1'000'000;

    Stopwatch sw;
    auto const print_time = [&sw]() { printf("%9.3f ms\n", sw.get_time_milli()); };

    printf("creating vectors: ");
    sw.start();

    intVec src(n_elements, 0);
    intVec dst1(n_elements, 0);
    intVec dst2(n_elements, 0);
    intVec dst3(n_elements, 0);
    print_time();


    f_list funcs
    {
        [&]() { process_vector_for(src, dst1); },
        [&]() { process_vector_stl(src, dst2); },
        [&]() { process_vector_par(src, dst2); },
    };


    printf("execute_seq():    ");
    sw.start();

    execute_seq(funcs);

    print_time();

    printf("execute_par():    ");
    sw.start();

    execute_par(funcs);

    print_time();
}
```

And here are the results.

```plaintext
creating vectors:     2.345 ms
execute_seq():     5296.348 ms
execute_par():     2803.655 ms
```

As expected the sequential version took as much time as all of them combined.

We would normally expect the parallel version to take only as much time as the longest process.  In this case however one of the functions is heavily parallelized itself, so the multithreading resources are shared.  The end result is a bit less time than one sequential version and the parallel version together.

If we omit the parallel version from our example, then we get what we expect.

```cpp
f_list funcs
{
    [&]() { process_vector_for(src, dst1); },
    [&]() { process_vector_stl(src, dst2); },
    //[&]() { process_vector_par(src, dst2); },
};
```

```plaintext
creating vectors:     2.201 ms
execute_seq():     4761.584 ms
execute_par():     2408.606 ms
```

### Still easy

In a previous [post](https://almostalwaysauto.com/posts/parallelism-for-free) we covered how `std::execution` can be used with several STL algorithms to process elements of a standard container in parallel.  Here we showed that we can execute functions in parallel by treating the functions themselves as elements of a standard container.  It even works when one or more of the functions are multithreaded as well.  It is slightly more work but still incredibly simple to implement.
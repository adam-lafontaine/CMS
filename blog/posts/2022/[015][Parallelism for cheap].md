# Parallelism for cheap
## 


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


```cpp
#include <algorithm>


void process_vector_stl(intVec& src, intVec& dst)
{
    assert(dst.size() == src.size());
    
    std::transform(src.begin(), src.end(), dst.begin(), do_work);
}
```


```cpp
#include <execution>


void process_vector_par(intVec& src, intVec& dst)
{
    assert(dst.size() == src.size());

    std::transform(std::execution::par, src.begin(), src.end(), dst.begin(), do_work);
}
```

```cpp
#include "stopwatch.hpp"


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


```plaintext
creating vectors:         1.530 ms
process_vector_for():  2377.826 ms
process_vector_stl():  2371.840 ms
process_vector_par():   487.976 ms
```

```cpp
#include <functional>


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

    std::for_each(std::execution::par, funcs.begin(), funcs.end(), [](auto const& func) { func(); });
}
```


```plaintext
Hello my A1 A2 B1 B2 C1 C2 D1 C3 C4 name is Adam D2 D3 D4 D5 C5 A3 A4 A5 B3 B4 B5
```


```cpp
using void_f_list = std::vector<std::function<void()>>;


void execute(void_f_list const& funcs)
{
    std::for_each(funcs.begin(), funcs.end(), [](auto const& func) { func(); });
}


void execute_par(void_f_list const& funcs)
{
    std::for_each(std::execution::par, funcs.begin(), funcs.end(), [](auto const& func) { func(); });
}
```


```cpp
void_f_list funcs
{
    [&]() {process_vector_for(src, dst); },
    [&]() {process_vector_stl(src, dst); },
    [&]() {process_vector_par(src, dst); },
};
```


```cpp
int main()
{
    u32 n_elements = 1'000'000;

    Stopwatch sw;
    auto const print_time = [&sw]() { printf("%9.3f ms\n", sw.get_time_milli()); };

    printf("creating vectors: ");
    sw.start();

    intVec src(n_elements, 0);
    intVec dst(n_elements, 0);
    print_time();


    void_f_list funcs
    {
        [&]() {process_vector_for(src, dst); },
        [&]() {process_vector_stl(src, dst); },
        [&]() {process_vector_par(src, dst); },
    };


    printf("execute():        ");
    sw.start();

    execute(funcs);

    print_time();

    printf("execute_par():    ");
    sw.start();

    execute_par(funcs);

    print_time();
}
```


```plaintext
creating vectors:     1.231 ms
execute():         5252.308 ms
execute_par():     2742.352 ms
```


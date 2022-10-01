# Parallelism for cheap
## 


```cpp
#include <random>

using u32 = unsigned;


constexpr u32 AMOUNT_OF_WORK = 100;


int do_work(int n)
{
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
#include "stopwatch.hpp"


int main()
{
    u32 n_elements = 1'000'000;

    Stopwatch sw;
    double t = 0.0;

    printf("creating vectors: ");
    sw.start();

    intVec src(n_elements, 0);
    intVec dst(n_elements, 0);

    t = sw.get_time_milli();
    printf("%f ms\n", t);

    printf("process_vector_for(): ");
    sw.start();

    process_vector_for(src, dst);

    t = sw.get_time_milli();
    printf("%f ms\n", t);

    printf("process_vector_stl(): ");
    sw.start();

    process_vector_stl(src, dst);

    t = sw.get_time_milli();
    printf("%f ms\n", t);

    u32 result = dst[0] == 5;
    printf("Done %d\n", result);
}
```


```plaintext
creating vectors:        1.066500 ms
process_vector_for(): 2427.183600 ms
process_vector_stl(): 2418.538700 ms
Done 0
```


```cpp
#include <execution>


void process_vector_stl_par(intVec& src, intVec& dst)
{
    assert(dst.size() == src.size());

    std::transform(std::execution::par, src.begin(), src.end(), dst.begin(), do_work);
}
```

```cpp
int main()
{
    u32 n_elements = 1'000'000;

    Stopwatch sw;
    double t = 0.0;

    printf("creating vectors:         ");
    sw.start();

    intVec src(n_elements, 0);
    intVec dst(n_elements, 0);

    t = sw.get_time_milli();
    printf("%11f ms\n", t);

    printf("process_vector_for():     ");
    sw.start();

    process_vector_for(src, dst);

    t = sw.get_time_milli();
    printf("%11f ms\n", t);

    printf("process_vector_stl():     ");
    sw.start();

    process_vector_stl(src, dst);

    t = sw.get_time_milli();
    printf("%11f ms\n", t);

    printf("process_vector_stl_par(): ");
    sw.start();

    process_vector_stl_par(src, dst);

    t = sw.get_time_milli();
    printf("%11f ms\n", t);

    u32 result = dst[0] == 5;
    printf("Done %d\n", result);
}
```


```plaintext
creating vectors:            1.574800 ms
process_vector_for():     2404.177500 ms
process_vector_stl():     2406.099200 ms
process_vector_stl_par():  479.078000 ms
Done 0
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


void execute_par(void_f_list const& funcs)
{
    std::for_each(std::execution::par, funcs.begin(), funcs.end(), [](auto const& func) { func(); });
}
```

```cpp
void process_vector_elements(intVec& src, intVec& dst, size_t id_begin, size_t id_end)
{
    assert(dst.size() == src.size());
    assert(id_end > id_begin);
    assert(id_end <= src.size());

    for (auto i = id_begin; i < id_end; ++i)
    {
        dst[i] = do_work(src[i]);
    }
}
```

```cpp
void process_vector_chunks_4(intVec& src, intVec& dst)
{
    assert(dst.size() == src.size());
    auto n_elements = src.size();

    auto chunk_size = n_elements / 4;

    void_f_list funcs
    {
        [&]() { process_vector_elements(src, dst, 0, chunk_size); },
        [&]() { process_vector_elements(src, dst, chunk_size, 2 * chunk_size); },
        [&]() { process_vector_elements(src, dst, 2 * chunk_size, 3 * chunk_size); },
        [&]() { process_vector_elements(src, dst, 3 * chunk_size, n_elements); },
    };

    execute_par(funcs);
}
```


```cpp
int main()
{
    u32 n_elements = 1'000'000;

    Stopwatch sw;
    auto const print_time = [&sw]() { printf("%11f ms\n", sw.get_time_milli()); };

    printf("creating vectors:          ");
    sw.start();

    intVec src(n_elements, 0);
    intVec dst(n_elements, 0);

    print_time();

    printf("process_vector_stl_par():  ");
    sw.start();

    process_vector_stl_par(src, dst);

    print_time();

    printf("process_vector_chunks_4(): ");
    sw.start();

    process_vector_chunks_4(src, dst);

    print_time();

    u32 result = dst[0] == 5;
    printf("Done %d\n", result);
}
```


```plaintext
creating vectors:             1.693700 ms
process_vector_stl_par():   551.831500 ms
process_vector_chunks_4():  775.644100 ms
Done 0
```


```plaintext
creating vectors:             12.984400 ms
process_vector_stl_par():   4896.962800 ms
process_vector_chunks_4():  7313.797600 ms
process_vector_chunks_8():  6268.880900 ms
process_vector_chunks_16(): 7722.539600 ms
Done 0
```

```cpp
#include <array>


template <size_t N>
void execute_par(std::array<std::function<void()>, N> const& funcs)
{
    std::for_each(std::execution::par, funcs.begin(), funcs.end(), [](auto const& func) { func(); });
}
```


```cpp
void process_vector_chunks_8(intVec& src, intVec& dst)
{
    assert(dst.size() == src.size());
    auto n_elements = src.size();

    auto chunk_size = n_elements / 8;

    std::array<std::function<void()>, 8> funcs
    //void_f_list funcs
    {
        [&]() { process_vector_elements(src, dst, 0, chunk_size); },
        [&]() { process_vector_elements(src, dst, chunk_size, 2 * chunk_size); },
        [&]() { process_vector_elements(src, dst, 2 * chunk_size, 3 * chunk_size); },
        [&]() { process_vector_elements(src, dst, 3 * chunk_size, 4 * chunk_size); },
        [&]() { process_vector_elements(src, dst, 4 * chunk_size, 5 * chunk_size); },
        [&]() { process_vector_elements(src, dst, 5 * chunk_size, 6 * chunk_size); },
        [&]() { process_vector_elements(src, dst, 6 * chunk_size, 7 * chunk_size); },
        [&]() { process_vector_elements(src, dst, 7 * chunk_size, n_elements); },
    };

    execute_par(funcs);
}
```

```plaintext
creating vectors:             11.210300 ms
process_vector_stl_par():   4941.146400 ms
process_vector_chunks_4():  7257.094400 ms
process_vector_chunks_8():  7484.346700 ms
process_vector_chunks_16(): 7779.026700 ms
Done 0
```


```cpp
int do_work(int n)
{
    /*std::random_device r;
    std::default_random_engine eng(r());
    std::uniform_int_distribution<int> uniform_dist(10, 1000);

    int sum = n;

    for (u32 i = 0; i < AMOUNT_OF_WORK; ++i)
    {
        sum += uniform_dist(eng) % 3;
    }

    return sum;*/

    return n * 2 + 3;
}
```
100,000,000 elements

```plaintext
creating vectors:          123.803100 ms
process_vector_for():       63.204300 ms
process_vector_stl():       68.267900 ms
process_vector_stl_par():   53.182300 ms
process_vector_chunks_4():  53.297200 ms
process_vector_chunks_8():  55.174700 ms
process_vector_chunks_16(): 55.345700 ms
Done 0
```

Without optimizations

```plaintext
creating vectors:          119.050400 ms
process_vector_for():       357.404100 ms
process_vector_stl():       190.443700 ms
process_vector_stl_par():   56.763100 ms
process_vector_chunks_4():  130.584700 ms
process_vector_chunks_8():  108.595600 ms
process_vector_chunks_16(): 108.132300 ms
Done 0
```
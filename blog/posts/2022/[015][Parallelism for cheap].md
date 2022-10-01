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
    auto n_elements = src.size();
    assert(dst.size() == n_elements);

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
}
```


```plaintext
creating vectors:        1.066500 ms
process_vector_for(): 2427.183600 ms
process_vector_stl(): 2418.538700 ms
```
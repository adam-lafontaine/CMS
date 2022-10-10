# Parallelism for free
## And algorithms too


### Standard Algorithms

There are a multitude of algorithms available in the C++ Standard Template Library (STL).  Cppreference is a good resource https://en.cppreference.com/w/cpp/algorithm.  There's no need to have them all memorized, but it's good to know what is available.  There is a good chance that what you need to do already exists.  It can save you a lot of time trying to implement it yourself, while making your code cleaner and more readable.

The STL algorithms operate on a range of elements specified by providing an iterator to the first element and another to one past the last element.  The standard containers have methods for `begin()` and `end()` respectively.  Consider the following example.

```cpp
#include <vector>
#include <algorithm>
#include <numeric>
#include <random>


constexpr unsigned AMOUNT_OF_WORK = 100;


int do_work(int n)
{
    std::random_device r;
    std::default_random_engine eng(r());
    std::uniform_int_distribution<int> uniform_dist(10, 1000);

    int sum = n;

    for (unsigned i = 0; i < AMOUNT_OF_WORK; ++i)
    {
        sum += uniform_dist(eng) % 3;
    }

    return sum;
}


void process_vector(std::vector<int>& src, std::vector<int>& dst)
{
    auto const times_two = [](int& n) { n *= 2; };

    // double each element in src
    std::for_each(src.begin(), src.end(), times_two);

    // apply the do_work function to each value of src and store the results in dst
    std::transform(src.begin(), src.end(), dst.begin(), do_work);
}
```

There is no real purpose to the do_work function.  It is simply supposed to take some time generating a result.  It accumulates the modulus calculation of 100 randomly generated numbers.  We can adjust the amount of time it takes with the AMOUNT_OF_WORK constant.

The process_vector function demonstrates the standard algorithms std::for_each and std::transform.  It does the following.
* Apply a lambda to each element of src, doubling them in place (https://en.cppreference.com/w/cpp/algorithm/for_each)
* Call the do_work function with each element of src and save the result in dst (https://en.cppreference.com/w/cpp/algorithm/transform)

Many people find it irritating to have to pass begin and end iterators to these functions instead of just the container like many other languages.  Sometimes they resort to writing wrapper functions to make their code more concise.  However this has the drawback of having to maintain another library.

```cpp
// https://en.cppreference.com/w/cpp/algorithm/sort

template <typename T>
void sort(T& container)
{
    std::sort(container.begin(), container.end());
}
```

Another option is the `std::ranges` namespace introduced in C++20.  That is a topic worthy of its own post, but one thing it allows for is more concise function calls.

```cpp
// https://en.cppreference.com/w/cpp/algorithm/ranges/sort

std::ranges::sort(vec);
```

In any case, begin and end iterators are still required to define the range we want to perform an algorithm on.

We also have the option to apply an algorithm to a subset of a range.  For example, we can sort only the first 10 elements of a containiner.

```cpp
std::sort(vec.begin(), vec.begin() + 10);
```

We can also sort only the last 10 elements.

```cpp
std::sort(vec.end() - 10, vec.end());
```

Or everthing but the first and last 10 elements.

```cpp
std::sort(vec.begin() + 10, vec.end - 10);
```

The intent for specifying the begin and end iterators is to provide flexibility and functionality instead of simplicity.


### Parallelism

If your collections are quite large or if there are many of them to process, operating on each element one at a time can be a bottleneck in your application.  C++17 introduced the ability for many algorithms to process several elements at the same time using multi-threading.  How this is executed will depend on the operating system and the computer's CPU.  What this means for developers is that this functionality is available without any extra programming.  We only need to pass a parameter indicating that we wish the algorithm to execute in parallel.

```cpp
#include <execution>


void process_vector_par(std::vector<int>& src, std::vector<int>& dst)
{
    auto const times_two = [](int& n) { n *= 2; };

    // double each element in src
    std::for_each(std::execution::par, src.begin(), src.end(), times_two);

    // apply the do_work function to each value of src and store the results in dst
    std::transform(std::execution::par, src.begin(), src.end(), dst.begin(), do_work);
}
```

This example is the same as the one above except that we passed the Execution Policy `std::execution::par` to `std::for_each` and `std::transform`.  This should be faster but we need to test it.


### Speed Test

The following class can be used for timing code execution.  Copy and save it to a file called stopwatch.hpp

```cpp
#pragma once
#include <chrono>

class Stopwatch
{
private:
	std::chrono::system_clock::time_point start_;
	std::chrono::system_clock::time_point end_;
	bool is_on_ = false;

	std::chrono::system_clock::time_point now() { return std::chrono::system_clock::now(); }

public:
	Stopwatch()
	{
		start_ = now();
		end_ = start_;
	}

	void start()
	{
		start_ = now();
		is_on_ = true;
	}

	void stop()
	{
		end_ = now();
		is_on_ = false;
	}

	double get_time_milli()
	{
		std::chrono::duration<double, std::milli> delay = is_on_ ? now() - start_ : end_ - start_;

		return delay.count();
	}

	double get_time_sec()
	{
		std::chrono::duration<double> delay = is_on_ ? now() - start_ : end_ - start_;

		return delay.count();
	}

};
```

This program runs each example and outputs their execution times.

```cpp
#include <iostream>

#include "stopwatch.hpp"


constexpr unsigned ELEMENT_COUNT = 100'000;


int main()
{
    Stopwatch sw;

    auto const print_time = [&](const char* msg) { std::cout << msg << ": " << sw.get_time_milli() << "ms\n"; };

    std::vector<int> src_vec(ELEMENT_COUNT);
    std::vector<int> dst_vec(ELEMENT_COUNT);

    // { 1, 2, 3, 4, ... , size }
    std::iota(src_vec.begin(), src_vec.end(), 1); // https://en.cppreference.com/w/cpp/algorithm/iota

    std::cout << '\n';

    sw.start();
    process_vector(src_vec, dst_vec);
    print_time("sequential");

    // reset src_vec
    std::iota(src_vec.begin(), src_vec.end(), 1);

    sw.start();
    process_vector_par(src_vec, dst_vec);
    print_time("  parallel");
}
```

Output when running in debug mode.

![alt text](https://github.com/adam-lafontaine/CMS/raw/current/blog/img/%5B004%5D/debug.png)

Output when running in release mode.

![alt text](https://github.com/adam-lafontaine/CMS/raw/current/blog/img/%5B004%5D/release.png)

In both cases, the sequential version took almost 5 times longer to execute.  The times in this example are still very small but they adds up when working with larger collections and when the processing is more intensive.  You can experiment with this yourself by changing the values of `AMOUNT_OF_WORK` and `ELEMENT_COUNT`.

You can check and see how it affects your computer's CPU usage by opening Task Manager (Ctrl+Shift+Esc) and selecting the Performance tab.

For instance, when I increase each value by a factor of 10 I get the following result.

```cpp
constexpr unsigned AMOUNT_OF_WORK = 1000;
constexpr unsigned ELEMENT_COUNT = 1000000;
```

![alt text](https://github.com/adam-lafontaine/CMS/raw/current/blog/img/%5B004%5D/cpu_usage.png)

While running sequencially, the CPU usage is at about 40%.  It then goes up to 100% when it does the parallel processing.  Parallel execution can be a CPU resource hog.  It could slow down other processes running on the computer, especially if they require significant CPU resources as well.  It's also impossible to know how much faster it will be if other processes will be running at the same time.

Executing algorithms in parallel is not always faster.  Sometimes the decreased execution time does not make up for the extra overhead required to set up the multi-threading.  It's good to try both with different levels of optimization.  Sometimes the compiler can make running your code in parallel unnecessary.  Parallel execution is not a magic bullet but it can be extremely useful. It is basically free in that it requires no extra work to implement.  
# Parallelism for cheap
## 


```cpp
#include <vector>


using r32 = float;
using u32 = unsigned;

using Vec32 = std::vector<r32>;


void vector_multiply_add(Vec32 const& a, Vec32 const& b, Vec32 const& c, Vec32& result, u32 index)
{
	result[index] = a[index] * b[index] + c[index];
}
```

```cpp
#include <cassert>


void vma_for(Vec32 const& a, Vec32 const& b, Vec32 const& c, Vec32& result)
{
	auto n_elements = result.size();
	assert(a.size() == n_elements);
	assert(b.size() == n_elements);
	assert(c.size() == n_elements);

	for (u32 i = 0; i < n_elements; ++i)
	{
		vector_multiply_add(a, b, c, result, i);
	}
}
```

```cpp
#include <cstdio>
#include <algorithm>


int main()
{
    u32 n_elements = 100'000'000;

	// 3 * 4 + 5 = 17
	printf("create vectors\n");
	Vec32 vec_3(n_elements, 3.0f);
	Vec32 vec_4(n_elements, 4.0f);
	Vec32 vec_5(n_elements, 5.0f);
	Vec32 vec_17(n_elements, 0.0f);

	printf("vma_for(): ");
	vma_for(vec_3, vec_4, vec_5, vec_17);
	
	bool result = std::all_of(vec_17.begin(), vec_17.end(), [](r32 val) { return fabs(val - 17.0f) < 1e-5; });
	if (result)
	{
		printf("PASS\n");
	}
	else
	{
		printf("FAIL %f, %f, %f, %f\n", vec_3[0], vec_4[0], vec_5[0], vec_17[0]);
	}
}
```


```plaintext
create vectors
vma_for(): PASS
```
# How to CUDA
## Parallel programming with NVIDIA GPUs


```cpp
#include <cstdio>
#include <locale>
#include <algorithm>
#include <cstdint>

using r32 = float;
using u32 = unsigned;

class FloatBuffer;

bool host_malloc(FloatBuffer& buffer, u32 n_elements);
void host_free(FloatBuffer& buffer);
bool device_malloc(FloatBuffer& buffer, u32 n_elements);
bool device_free(FloatBuffer& buffer);
r32* push_elements(FloatBuffer& buffer, u32 n_elements);

bool memcpy_to_device(const void* host_src, void* device_dst, size_t n_bytes);
bool memcpy_to_host(const void* device_src, void* host_dst, size_t n_bytes);

bool launch_kernel(r32* a, r32* b, r32* c, r32* result, u32 n_elements);


int main()
{
    u32 n_elements = 1'000'000;
    u32 total_elements = 4 * n_elements;

    setlocale(LC_NUMERIC, "");
    printf("\nMultiply-Add %'d elements\n", n_elements);

    FloatBuffer host_buffer{};
    FloatBuffer device_buffer{};

    auto const allocate = [&]()
    {
        return
            host_malloc(host_buffer, total_elements) &&
            device_malloc(device_buffer, total_elements);
    };

    auto const cleanup = [&]()
    {
        host_free(host_buffer);
        device_free(device_buffer);
    };

    printf("allocate memory\n");
    if(!allocate())
    {
        cleanup();
        return 1;
    }

    // 3 * 4 + 5 = 17
    printf("create host arrays\n");    
    r32* host_3 = push_elements(host_buffer, n_elements);
    r32* host_4 = push_elements(host_buffer, n_elements);
    r32* host_5 = push_elements(host_buffer, n_elements);
    r32* host_17 = push_elements(host_buffer, n_elements);

    for(u32 i = 0; i < n_elements; ++i)
    {
        host_3[i] = 3;
        host_4[i] = 4;
        host_5[i] = 5;
    }

    printf("create device arrays\n");
    r32* device_3 = push_elements(device_buffer, n_elements);
    r32* device_4 = push_elements(device_buffer, n_elements);
    r32* device_5 = push_elements(device_buffer, n_elements);
    r32* device_17 = push_elements(device_buffer, n_elements);

    // copy the first 3 arrays in one call
    printf("copy host arrays to device\n");
    auto bytes_to_copy = 3 * n_elements * sizeof(r32);
    bool copy = memcpy_to_device(host_buffer.data, device_buffer.data, bytes_to_copy);
    if(!copy)
    {
        cleanup();
        return 1;
    }

    printf("launch kernel\n");
    bool launch = launch_kernel(device_3, device_4, device_5, device_17, n_elements);
    if(!launch)
    {
        cleanup();
        return 1;
    }

    printf("copy device results to host\n");
    bytes_to_copy = n_elements * sizeof(r32);
    copy = memcpy_to_host(device_17, host_17, bytes_to_copy);
    if(!copy)
    {
        cleanup();
        return 1;
    }

    printf("check results\n");
    auto begin = host_17;
    auto end = begin + n_elements;
    auto result = std::all_of(begin, end, [](r32 val){ return fabs(val - 17.0f) < 1e-5; });

    if(result)
    {
        printf("PASS\n");
    }
    else
    {
        printf("FAIL %f, %f, %f, %f\n", host_3[0], host_4[0], host_5[0], host_17[0]);
    }

    printf("free memory\n");
    cleanup();
    return 0;
}
```

* Allocate memory on the computer and the GPU
* Initialize arrays on the computer with data for processing
* Initialize arrays on the GPU
* Copy the data to the memory on the GPU
* Process the data on the GPU
* Copy the results from the GPU to the computer
* Verify the results

```plaintext
Multiply-Add 1,000,000 elements
allocate memory
create host arrays
create device arrays
copy host arrays to device
launch kernel
copy device results to host
check results
PASS
free memory
```

### Memory management

```cpp
FloatBuffer host_buffer{};
FloatBuffer device_buffer{};

auto const allocate = [&]()
{
    return
        host_malloc(host_buffer, total_elements) &&
        device_malloc(device_buffer, total_elements);
};

auto const cleanup = [&]()
{
    host_free(host_buffer);
    device_free(device_buffer);
};

printf("allocate memory\n");
if(!allocate())
{
    cleanup();
    return 1;
}
```


```cpp
class FloatBuffer
{
public:
    u32 capacity = 0;
    u32 size = 0;

    r32* data = nullptr;
};
```


```cpp
bool host_malloc(FloatBuffer& buffer, u32 n_elements)
{
    if(!n_elements || buffer.data)
    {
        return false;
    }

    auto n_bytes = n_elements * sizeof(r32);

    buffer.data = (r32*)malloc(n_bytes);
    if(buffer.data)
    {
        buffer.capacity = n_elements;
    }

    return true;
}


void host_free(FloatBuffer& buffer)
{   
    buffer.capacity = 0;
    buffer.size = 0;

    if(buffer.data)
    {
        free(buffer.data);
        buffer.data = nullptr;
    }
}
```


```cpp
#include <cuda_runtime.h>


bool device_malloc(FloatBuffer& buffer, u32 n_elements)
{
    if(!n_elements || buffer.data)
    {
        return false;
    }

    auto n_bytes = n_elements * sizeof(r32);

    cudaError_t err = cudaMalloc((void**)&(buffer.data), n_bytes);
    check_error(err);

    auto result = buffer.data && err == cudaSuccess;
    if(result)
    {
        buffer.capacity = n_elements;
    }

    return result;
}


bool device_free(FloatBuffer& buffer)
{
    buffer.capacity = 0;
    buffer.size = 0;

    if(buffer.data)
    {
        cudaError_t err = cudaFree(buffer.data);
        check_error(err);

        buffer.data = nullptr;

        return err == cudaSuccess;
    }

    return true;
}
```

### Error checking

```cpp
void check_error(cudaError_t err)
{
    if(err == cudaSuccess)
    {
        return;
    }

    printf("\n*** CUDA ERROR ***\n\n");
    printf("%s", cudaGetErrorString(err));
    
    printf("\n\n******************\n\n");
}
```


```plaintext
Multiply-Add 1,000,000,000 elements
allocate memory

*** CUDA ERROR ***

out of memory

******************
```

### Memory access

```cpp
// 3 * 4 + 5 = 17
printf("create host arrays\n");    
r32* host_3 = push_elements(host_buffer, n_elements);
r32* host_4 = push_elements(host_buffer, n_elements);
r32* host_5 = push_elements(host_buffer, n_elements);
r32* host_17 = push_elements(host_buffer, n_elements);

for(u32 i = 0; i < n_elements; ++i)
{
    host_3[i] = 3;
    host_4[i] = 4;
    host_5[i] = 5;
}

printf("create device arrays\n");
r32* device_3 = push_elements(device_buffer, n_elements);
r32* device_4 = push_elements(device_buffer, n_elements);
r32* device_5 = push_elements(device_buffer, n_elements);
r32* device_17 = push_elements(device_buffer, n_elements);
```


```cpp
r32* push_elements(FloatBuffer& buffer, u32 n_elements)
{
    assert(buffer.data);
    assert(buffer.capacity);
    assert(buffer.size < buffer.capacity);

    auto is_valid = 
        buffer.data &&
        buffer.capacity &&
        buffer.size < buffer.capacity;

    auto bytes_available = (buffer.capacity - buffer.size) >= n_elements;
    assert(bytes_available);

    if(!is_valid || !bytes_available)
    {
        return nullptr;
    }

    auto data = buffer.data + buffer.size;
    buffer.size += n_elements;

    return data;
}
```


```cpp
// copy the first 3 arrays in one call
printf("copy host arrays to device\n");
auto bytes_to_copy = 3 * n_elements * sizeof(r32);
auto copy = memcpy_to_device(host_buffer.data, device_buffer.data, bytes_to_copy);
if(!copy)
{
    cleanup();
    return 1;
}
```


```cpp
bool memcpy_to_device(const void* host_src, void* device_dst, size_t n_bytes)
{
    assert(host_src);
    assert(device_dst);

    cudaError_t err = cudaMemcpy(device_dst, host_src, n_bytes, cudaMemcpyHostToDevice);
    check_error(err);

    return err == cudaSuccess;
}
```


```cpp
printf("launch kernel\n");
auto launch = launch_kernel(device_3, device_4, device_5, device_17, n_elements);
if(!launch)
{
    cleanup();
    return 1;
}
```


```cpp
printf("copy device results to host\n");
bytes_to_copy = n_elements * sizeof(r32);
copy = memcpy_to_host(device_17, host_17, bytes_to_copy);
if(!copy)
{
    cleanup();
    return 1;
}
```


```cpp
printf("check results\n");
auto begin = host_17;
auto end = begin + n_elements;
auto result = std::all_of(begin, end, [](r32 val){ return fabs(val - 17.0f) < 1e-5; });

if(result)
{
    printf("PASS\n");
}
else
{
    printf("FAIL %f, %f, %f, %f\n", host_3[0], host_4[0], host_5[0], host_17[0]);
}

printf("free memory\n");
cleanup();
return 0;
```


### GPU programming


```cpp
/**
 * CUDA Kernel Device code
 *
 * Computes the vector addition of A and B into C. The 3 vectors have the same
 * number of elements numElements.
 */
__global__ void
vectorAdd(const float *A, const float *B, float *C, int numElements)
{
    int i = blockDim.x * blockIdx.x + threadIdx.x;

    if (i < numElements)
    {
        C[i] = A[i] + B[i];
    }
}
```

```cpp
#define GPU_KERNAL __global__
#define GPU_FUNCTION __device__


GPU_FUNCTION
r32 multipy_add(r32 a, r32 b, r32 c)
{
    return a * b + c;
}


GPU_KERNAL
void gpu_multiply_add(r32* a, r32* b, r32* c, r32* result, u32 n_elements)
{
    int i = blockDim.x * blockIdx.x + threadIdx.x;

    if (i >= n_elements)
    {
        return;
    }

    result[i] = multipy_add(a[i], b[i], c[i]);
}
```


```cpp
bool launch_kernel(r32* a, r32* b, r32* c, r32* result, u32 n_elements)
{
    constexpr int THREADS_PER_BLOCK = 1024;

    auto n_threads = n_elements;
    int n_blocks = (n_threads + THREADS_PER_BLOCK - 1) / THREADS_PER_BLOCK;

    gpu_multiply_add <<<n_blocks, THREADS_PER_BLOCK>>> (a, b, c, result, n_elements);    

    cudaError_t err = cudaDeviceSynchronize();
    check_error(err);

    return err == cudaSuccess;
}
```
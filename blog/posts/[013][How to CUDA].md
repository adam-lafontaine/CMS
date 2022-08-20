# How to CUDA
## Parallel programming with Nvidia

Graphical Processing Units (GPUs) are no longer just for computer graphics.  Computer graphics is essentially a series of fairly simple linear algebra calculations performed for each pixel in a window.  Rather than performing these pixel calculations in sequence on a CPU, GPU's were developed so that identical calculations could be performed for each pixel in parallel.  Each "mini" processor on a GPU is nowhere near as powerful as a modern CPU but the volume of data that can be processed at once more than makes up for it.  

In this post we'll perform a multiply-add operation using arrays of one million elements while covering some of the CUDA api that allows our programs to interact with a GPU.

Actually executing the program however will not be covered here.  Nvidia has its own compiler which works in much the same way as other C++ compilers but has its differences.  Compiling and running different types of programs on different platforms will be a topic for a later post.

Getting up and running is not difficult though.  If your computer has a Nvidia GPU, you can get started by downloading and installing the CUDA toolkit.

https://developer.nvidia.com/cuda-downloads

### Example program

Below is our main program along with definitions for some helper functions.

```cpp
#include <cstdio>
#include <locale>
#include <algorithm>
#include <cstdint>

using r32 = float;
using u32 = unsigned;

// Helpers and api wrappers
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
    bool result = std::all_of(begin, end, [](r32 val){ return fabs(val - 17.0f) < 1e-5; });

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

Each step of the program will be explained in turn.
* Allocate memory on the computer and the GPU
* Initialize arrays on the computer with data for processing
* Initialize arrays on the GPU
* Copy the data to the memory on the GPU
* Process the data on the GPU
* Copy the results from the GPU to the computer
* Verify the results

If everything works as it should, the output will look like so.

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

The first step is to allocate memory on the host and the device.  The GPU is referred to as the device and the machine with the CPU is referred to as the host.  Device and host memory are separate as they each reside different pieces of hardware.  i.e. device memory on the GPU and host memory in RAM.

The first part of the program defines a couple of lamdas that allocate and free memory using `FloatBuffer` objects.

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

The `FloatBuffer` is just a container for managing a buffer of floats in memory.  One is used for host data and the other for device data.

```cpp
class FloatBuffer
{
public:
    u32 capacity = 0;
    u32 size = 0;

    r32* data = nullptr;
};
```

Host memory is handled using `malloc()` and `free()` as usual.

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

Allocating memory on the the device requires a CUDA api function called `cudaMalloc()`.  It takes a reference to a pointer variable and the number of bytes to allocate.  If successful, it sets the pointer to the address on the device where the memory has been allocated.

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
```

If the call is not successful, `cudaMalloc()` will return a `cudaError_t` value that is other than the constant `cudaSuccess`.  Most if not all of the CUDA api functions return `cudaError_t`.  Checking error codes will be covered in a bit.

Device memory also needs to be freed when we're done with it.  We do that with `cudaFree()`.

```cpp
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

Generally it's a good idea to check for errors after each api call.  How error handling should be done depends on the application and if it is still in development or production.  For our example, we'll simply print the error to the console with `cudaGetErrorString()`.

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

To force an error, we can attempt to process more data than what the GPU can handle.  I am using a Jetson Nano, so 4 arrays of one billion 32 bit elements does the trick.

```plaintext
Multiply-Add 1,000,000,000 elements
allocate memory

*** CUDA ERROR ***

out of memory

******************
```

### Memory access

The next step in the program is to divide the host and device memory into four equally sized arrays.  Host data is initialized with values that will be copied to the device for processing.

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

The host and device do not have access to each other's memory.  However, `cudaMalloc()` provides the device memory address and device pointer arithmetic is allowed on the host.  So assigning addresses to each array can be done in the same way for the host and the device data.

```cpp
#include <cassert>


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

In order for data to be processed on the GPU, it must first be copied to device memory.  Here we are copying all three arrays in one shot because their memory is contiguous and starts at the beginning of the buffer.

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

Copying memory from host to device is done using `cudaMemcpy()` with the constant `cudaMemcpyHostToDevice`.  Provide the address of where the data resides on the host and the address of where to copy to on the device with the number of bytes to copy.

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

When the data has been copied to the device it can be processed.  Device "kernels" are called from the host to initiate the parallel processing done by the GPU.  This will be covered in more detail later.

```cpp
printf("launch kernel\n");
bool launch = launch_kernel(device_3, device_4, device_5, device_17, n_elements);
if(!launch)
{
    cleanup();
    return 1;
}
```

After processing, the results need to be copied from the device to the host.

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

Copying from device to host is also done with `cudaMemcpy()`.  The device address of the data and the host address where it should be copied to need to be provided along with the number of bytes to copy.  The constant `cudaMemcpyDeviceToHost` tells the api that we are copying data from device memory to host memory.

```cpp
bool memcpy_to_host(const void* device_src, void* host_dst, size_t n_bytes)
{
    assert(device_src);
    assert(host_dst);

    cudaError_t err = cudaMemcpy(host_dst, device_src, n_bytes, cudaMemcpyDeviceToHost);
    check_error(err);

    return err == cudaSuccess;
}
```

Finally, the program checks all of the results to make sure they are what we expect.

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

### Device kernels

Before seeing how our program processes data on the GPU, we'll first check out an example provided by Nvidia.

```cpp
/**
 * CUDA Kernel Device code
 *
 * Computes the vector addition of A and B into C. The 3 vectors have the same
 * number of elements numElements.
 */
__global__
void vectorAdd(const float *A, const float *B, float *C, int numElements)
{
    int i = blockDim.x * blockIdx.x + threadIdx.x;

    if (i < numElements)
    {
        C[i] = A[i] + B[i];
    }
}
```

The `__global__` declaration specifier informs Nvidia's compiler that the function is a "device kernel" and therefore executes in parallel with the number of threads specifed where it is called.

Threads are set up in a grid of thread blocks.  The number of threads in each block and the number of blocks are specifed at kernel launch.  The total number threads is the product of the two.  Since we want to process each array index on its own thread, the number of threads must be at least as many as the number of elements in each array.

Each thread has its own id and is retreived from the thread blocks like so.

```cpp
int i = blockDim.x * blockIdx.x + threadIdx.x;
```

For convienence, blocks can be one, two, or three-dimensional to allow for simple element lookup in multi-dimensional data structures.  The built-in variables `blockDim`, `blockIdx`, and `threadIdx` allow for finding the thread id based on how the blocks of threads are set up.

Since the number of threads can be greater than the number of elements to process, we need to make sure that the given thread id is within the bounds of the array.

```cpp
if (i < numElements)
{
    ...
}
```

The kernel is called from the host code like so.

```cpp
// vectorAdd example
int main()
{
    ...

    // Launch the Vector Add CUDA Kernel
    int threadsPerBlock = 256;
    int blocksPerGrid = (numElements + threadsPerBlock - 1) / threadsPerBlock;
    printf("CUDA kernel launch with %d blocks of %d threads\n", blocksPerGrid, threadsPerBlock);

    vectorAdd<<<blocksPerGrid, threadsPerBlock>>>(d_A, d_B, d_C, numElements);
    err = cudaGetLastError();

    ...
}
```

A kernel is launched using the execution configuration syntax (`<<<...>>>`) where the number of blocks and the threads per block are specified.  The api function `cudaGetLastError()` returns an error code if there were any problems during kernel execution.

### Device functions

We can call functions from device code as long as we use the `__device__` declaration specifier.  For example, we can modify the vectorAdd example by having the kernel call a function that adds two values.

```cpp
__device __
float add(float a, float b)
{
    return a + b;
}


__global__
void vectorAdd(const float *A, const float *B, float *C, int numElements)
{
    int i = blockDim.x * blockIdx.x + threadIdx.x;

    if (i < numElements)
    {
        C[i] = add(A[i], B[i]);
    }
}
```

### Back to our program

Double underscores are not very aesthetically pleasing, so lets define the following macros.

```cpp
#define GPU_KERNAL __global__
#define GPU_FUNCTION __device__
```

Our device code will consist of a function that performs a multiply and add and a kernel that calls the function for a given thread id / array index.

```cpp
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

Our launch_kernel() function that's called from main() sets up the thread blocks, launches the kernel, and checks for errors.

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

A thread block can consist of a maximum of 1024 threads.  The number of blocks is calculated to be the minimum number of blocks required given the number of threads in each block.

Pointers to the device data are passed to the kernel for processing.

This time error checking is done using `cudaDeviceSynchronize()`.  This blocks the program until all device execution completes.  It returns an error code if there were any problems.  It isn't really necessary here, but it is useful during development.  Device code execution is invisible to the host.  Simply launching a kernel will not throw any errors.  The CUDA runtime will record an error code and only return the last error when it is requested.

### Parallelism is not free

As you can see, it is quite a bit of work just to perform a simple operation.  The payoff is that we are able to do the operation on a very large data set all at once.  More work would be required to get the code running optimally in the context of a larger program.  This code is far from optimal but the first step in development is always to just get it working.

During development it is wise to check for errors after every api call and kernel launch in order catch any errors as soon as possible.  This has a performance cost but when we are confident that the application works, we can relax on the error handling and start optimizing in order to get the most out of the GPU.

As with anything, there is work involved making a GPU compatible program.  For an idea of how much work, check out the CUDA programming guide.  

https://docs.nvidia.com/cuda/pdf/CUDA_C_Programming_Guide.pdf

It is 454 pages long as of the writing of this post.
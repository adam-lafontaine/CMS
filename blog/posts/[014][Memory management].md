# Memory management
## It's not hard

### Pointers

Pointers are scary.  Or at least that's what we're told.  I remember in university when we were first taught about pointers.  It almost broke everyone's brain.  Looking back I can see now that the professor just didn't do a very good job of explaining them.  I don't blame him.  Pointers are hard to explain succinctly.  These days there are plenty of online resources available, but it was only with practical experience that I was able to just get it.  Perhaps I'll do a post dedicated to explaining pointers, but my advice is and always will be to just get to work.  Programming is not an academic pursuit, it is a skill.  And skills require practice.

### It's about performance

The fear of pointers is quite common and has been used to justify all kinds of language constructs that are supposodly safer but always end up with reduced and non-deterministic performance.  Automatic memory management such as garbage collection and even RAII (constructors/destructors) make frequent calls to `malloc()` and `free()`, holding up execution while the operating system deals with RAM.  Minimizing these calls is not difficult but can be inconvenient if you are used to programming with higher level languages.

### Example

The demonstration in this post will teach you everything you need to know about memory management.  After reading this, you'll never want a garbage collector again.

We can start with a simple struct that contains the address of the allocated memory (`data`), the amount of memory allocated (`capacity`) and the amount of the memory currently being used (`size`).  These are the only three things that need to be tracked.

```cpp
#include <cstdint>

using u8 = uint8_t;

class ByteBuffer
{
public:
    u8* data = nullptr;
	size_t capacity = 0;
	size_t size = 0;
};
```

To initialize the buffer, allocate a given number of bytes and set the memory address and capacity.

```cpp
#include <cstdlib>

bool buffer_create(ByteBuffer& buffer, size_t n_bytes)
{
    auto data = std::malloc(n_bytes);
    assert(data);

    buffer.data = (u8*)data;
    buffer.capacity = n_bytes;

    return data ? true : false;
}
```

When finished, free the memory and set the capacity and size back to zero.

```cpp
void buffer_destroy(ByteBuffer& buffer)
{
    if (buffer.data)
    {
        std::free(buffer.data);
        buffer.data = nullptr;
    }

    buffer.capacity = 0;
    buffer.size = 0;
}
```

The address in memory that is available for use is the address of `data` offset by the current `size`.  Whenever we want to use memory at the current available address, we need to update the `size` property by the amount of bytes we want.  It's as simple as getting the address, updating the `size`, and then returning the address.

```cpp
u8* buffer_push(ByteBuffer& buffer, size_t n_bytes)
{
    auto data = buffer.data + buffer.size;
    assert(data);

    buffer.size += n_bytes;

    return data;
}
```

It's better practice to first validate the current state of the buffer before returning the available address.

```cpp
u8* buffer_push(ByteBuffer& buffer, size_t n_bytes)
{
    assert(buffer.data);
    assert(buffer.capacity);
    assert(buffer.size < buffer.capacity);

    auto is_valid =
        buffer.data &&
        buffer.capacity &&
        buffer.size < buffer.capacity;

    auto bytes_available = (buffer.capacity - buffer.size) >= n_bytes;
    assert(bytes_available);

    if (!is_valid || !bytes_available)
    {
        return nullptr;
    }

    auto data = buffer.data + buffer.size;
    assert(data);

    buffer.size += n_bytes;

    return data;
}
```

We can avoid having to frequently reallocate memory by simply reusing what is currently allocated.  To write at the start of the buffer again, simply set the `size` property to zero.

```cpp
void buffer_reset(ByteBuffer& buffer)
{
    buffer.size = 0;
}
```

And that's it.  That is everything you need to know about memory management.  This example is the simplest that I could think of and is often good enough.  It will not work in all situations, but if you understand how it works you'll always be able implement whatever is required.

### Class implementation

Having separate functions is good for demonstration purposes but can be cumbersome in practice.  For convenience, here is a templated C++ class that can be used for any type.

```cpp
template <typename T>
class MemoryBuffer
{
private:
	T* data_ = nullptr;
	size_t capacity_ = 0;
	size_t size_ = 0;

public:
	MemoryBuffer(size_t n_elements)
	{
		auto data = std::malloc(sizeof(T) * n_elements);
		assert(data);

		data_ = (T*)data;
		capacity_ = n_elements;
	}


	T* push(size_t n_elements)
	{
		assert(data_);
		assert(capacity_);
		assert(size_ < capacity_);

		auto is_valid =
			data_ &&
			capacity_ &&
			size_ < capacity_;

		auto elements_available = (capacity_ - size_) >= n_elements;
		assert(elements_available);

		if (!is_valid || !elements_available)
		{
			return nullptr;
		}

		auto data = data_ + size_;
        assert(data);

		size_ += n_elements;

		return data;
	}


	void reset()
	{
		size_ = 0;
	}


	void free()
	{
		if (data_)
		{
			std::free(data_);
			data_ = nullptr;
		}

		capacity_ = 0;
		size_ = 0;
	}

    ~MemoryBuffer() { free(); }
};
```

### Memory safety

Many programming paradigms justify automated memory management by claiming that direct memory access is "unsafe".  I find this claim hard to justify because managing memory is so easy and making sure your code is "safe" only requires a little bit of discipline.  After all, the purpose of a software application is to make the CPU directly access and modify memory.  Why not just do it ourselves and give the users of our software a better experience?
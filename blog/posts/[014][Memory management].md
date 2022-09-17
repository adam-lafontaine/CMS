# Memory management
## It's not hard

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

```cpp
bool buffer_create(ByteBuffer& buffer, size_t n_bytes)
{
    auto data = std::malloc(sizeof(T) * n_elements);
    assert(data);

    buffer.data = (u8*)data;
    buffer.capacity = n_elements;

    return data ? true : false;
}
```

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

```cpp
u8* buffer_push(ByteBuffer& buffer, size_t n_bytes)
{
    auto data = buffer.data + buffer.size;
    assert(data);

    buffer.size += n_elements;

    return data;
}
```

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

    buffer.size += n_elements;

    return data;
}
```

```cpp
void buffer_reset(ByteBuffer& buffer)
{
    buffer.size = 0;
}
```

```cpp
#include <cstdlib>

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
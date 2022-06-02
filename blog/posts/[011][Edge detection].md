# Edge Detection
## 

```cpp

```

```cpp
#include <cstdint>

using u8 = uint8_t;
using u32 = uint32_t;


class Image
{
public:

	u32 width;
	u32 height;

	u8* data;
};


void make_image(Image& image, u32 width, u32 height)
{
	assert(width);
	assert(height);

	image.width = width;
	image.height = height;
	image.data = (u8*)malloc(sizeof(u8) * width * height);

	assert(image.data);
}


void dispose_image(Image& image)
{
	if (image.data != nullptr)
	{
		free(image.data);
		image.data = nullptr;
	}
}
```

```cpp
#include <array>

using r32 = float;


constexpr std::array<r32, 9> GRAD_X_3X3
{
	-0.25f,  0.0f,  0.25f,
	-0.50f,  0.0f,  0.50f,
	-0.25f,  0.0f,  0.25f,
};


constexpr std::array<r32, 9> GRAD_Y_3X3
{
	-0.25f, -0.50f, -0.25f,
	 0.0f,   0.0f,   0.0f,
	 0.25f,  0.50f,  0.25f,
};
```

```cpp
u8* row_begin(Image const& image, u32 y)
{
	auto row_offset = y * image.width;

	return image.data + row_offset;
}
```

```cpp
u8 pixel_at(Image const& image, u32 x, u32 y)
{
	return row_begin(image, y)[x];
}
```

```cpp
bool is_outer_pixel(Image const& img, u32 x, u32 y)
{
	return
		x == 0 ||
		x == img.width - 1 ||
		y == 0 ||
		y == img.height - 1;
}
```

```cpp
#include <cmath>
#include <cassert>


r32 gradient_at(Image const& img, u32 x, u32 y)
{
	if (is_outer_pixel(img, x, y))
	{
		return 0.0f;
	}

	r32 grad_x = 0.0f;
	r32 grad_y = 0.0f;
	u32 w = 0;

	for (u32 v = y - 1; v <= y + 1; ++v)
	{
		for (u32 u = x - 1; u <= x + 1; ++u)
		{
			auto p = pixel_at(img, u, v);

			grad_x += GRAD_X_3X3[w] * p;
			grad_y += GRAD_Y_3X3[w] * p;
			++w;
		}
	}

	auto grad = std::hypot(grad_x, grad_y);

	assert(grad >= 0.0f);
	assert(grad <= 255.0f);

	return grad;
}
```

```cpp
void gradients(Image const& src, Image const& dst)
{
	for (u32 y = 0; y < src.height; ++y)
	{
		auto dst_row = row_begin(dst, y);

		for (u32 x = 0; x < src.width; ++x)
		{
			auto grad = gradient_at(src, x, y);

			dst_row[x] = (u8)grad;
		}
	}
}
```

```cpp
int main()
{
    Image src_image;
	read_image_from_file("orange-car.bmp", src_image);

	Image dst_image;
	make_image(dst_image, src_image.width, src_image.height);

	gradients(src_image, dst_image);

	write_image(dst_image, "gradients.bmp");

	dispose_image(src_image);
	dispose_image(dst_image);
}
```


```cpp
u8 edge_at(Image const& img, u32 x, u32 y, u8 threshold)
{
	if (is_outer_pixel(img, x, y))
	{
		return 0.0f;
	}

	r32 grad_x = 0.0f;
	r32 grad_y = 0.0f;
	u32 w = 0;

	for (u32 v = y - 1; v <= y + 1; ++v)
	{
		for (u32 u = x - 1; u <= x + 1; ++u)
		{
			auto p = pixel_at(img, u, v);

			grad_x += GRAD_X_3X3[w] * p;
			grad_y += GRAD_Y_3X3[w] * p;
			++w;
		}
	}

	auto grad = std::hypot(grad_x, grad_y);

	assert(grad >= 0.0f);
	assert(grad <= 255.0f);

	return (u8)grad > threshold ? 255 : 0;
}
```

```cpp
void edges(Image const& src, Image const& dst, u8 threshold)
{
	for (u32 y = 0; y < src.height; ++y)
	{
		auto dst_row = row_begin(dst, y);

		for (u32 x = 0; x < src.width; ++x)
		{
			auto edge = edge_at(src, x, y, threshold);

			dst_row[x] = edge;
		}
	}
}
```

```cpp
int main()
{
    Image src_image;
	read_image_from_file("orange-car.bmp", src_image);

	Image dst_image;
	make_image(dst_image, src_image.width, src_image.height);

	gradients(src_image, dst_image);

	write_image(dst_image, "gradients.bmp");

	edges(src_image, dst_image, 60);

	write_image(dst_image, "edges.bmp");

	dispose_image(src_image);
	dispose_image(dst_image);
}
```
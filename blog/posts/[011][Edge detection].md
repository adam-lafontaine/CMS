# Edge Detection
## Calculating image gradients




```cpp
#include <cstdint>
#include <cassert>

using u8 = uint8_t;
using u32 = uint32_t;


class Image
{
public:

	u32 width = 0;
	u32 height = 0;

	u8* data = nullptr;
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
void zero_outer(Image const& dst)
{
	auto top_row = row_begin(dst, 0);
	auto bottom_row = row_begin(dst, dst.height - 1);

	for (u32 x = 0; x < dst.width; ++x)
	{
		top_row[x] = 0;
		bottom_row[x] = 0;
	}

	for (u32 y = 1; y < dst.height - 1; ++y)
	{
		auto dst_row = row_begin(dst, y);
		dst_row[0] = 0;
		dst_row[dst.width - 1] = 0;
	}
}
```

```cpp
#include <array>

using r32 = float;


constexpr std::array<r32, 9> GRAD_LR_3X3
{
	-0.25f,  0.0f,  0.25f,
	-0.50f,  0.0f,  0.50f,
	-0.25f,  0.0f,  0.25f,
};
```

```cpp
r32 x_gradient_at(Image const& img, u32 x, u32 y)
{
	r32 grad_lr = 0.0f;

	// x, y range
	auto x_begin = x - 1;
	auto x_end = x + 2;
	auto y_begin = y - 1;
	auto y_end = y + 2;

	u32 a = 0; // array index

	for (u32 v = y_begin; v < y_end; ++v)
	{
		for (u32 u = x_begin; u < x_end; ++u)
		{
			auto p = pixel_at(img, u, v);

			grad_lr += GRAD_LR_3X3[a] * p;
			++a;
		}
	}

	return std::abs(grad_lr);
}
```

```cpp
void x_gradients(Image const& src, Image const& dst)
{
	zero_outer(dst);

	auto x_begin = 1;
	auto x_end = src.width - 1;
	auto y_begin = 1;
	auto y_end = src.height - 1;

	for (u32 y = y_begin; y < y_end; ++y)
	{
		auto dst_row = row_begin(dst, y);

		for(u32 x = x_begin; x < x_end; ++x)
		{
			auto grad = x_gradient_at(src, x, y);

			assert(grad >= 0.0f);
			assert(grad <= 255.0f);

			dst_row[x] = (u8)grad;
		}
	}
}
```

```cpp
int main()
{
    Image chess_board_src;
	read_image_from_file("chess_board.bmp", chess_board_src);

	Image chess_board_dst;
	make_image(chess_board_dst, chess_board_src.width, chess_board_src.height);

	x_gradients(chess_board_src, chess_board_dst);
	write_image(chess_board_dst, "x_gradients.bmp");

	dispose_image(chess_board_src);
	dispose_image(chess_board_dst);
}
```

```cpp
constexpr std::array<r32, 9> GRAD_TB_3X3
{
	-0.25f, -0.50f, -0.25f,
	 0.0f,   0.0f,   0.0f,
	 0.25f,  0.50f,  0.25f,
};
```

```cpp
r32 y_gradient_at(Image const& img, u32 x, u32 y)
{
	r32 grad_tb = 0.0f;

	// x, y range
	auto x_begin = x - 1;
	auto x_end = x + 2;
	auto y_begin = y - 1;
	auto y_end = y + 2;

	u32 a = 0; // array index

	for (u32 v = y_begin; v < y_end; ++v)
	{
		for (u32 u = x_begin; u < x_end; ++u)
		{
			auto p = pixel_at(img, u, v);

			grad_tb += GRAD_TB_3X3[a] * p;
			++a;
		}
	}

	return std::abs(grad_tb);
}


void y_gradients(Image const& src, Image const& dst)
{
	zero_outer(dst);

	auto x_begin = 1;
	auto x_end = src.width - 1;
	auto y_begin = 1;
	auto y_end = src.height - 1;

	for (u32 y = y_begin; y < y_end; ++y)
	{
		auto dst_row = row_begin(dst, y);

		for (u32 x = x_begin; x < x_end; ++x)
		{
			auto grad = y_gradient_at(src, x, y);

			assert(grad >= 0.0f);
			assert(grad <= 255.0f);

			dst_row[x] = (u8)grad;
		}
	}
}
```

```cpp
int main()
{
    Image chess_board_src;
	read_image_from_file("chess_board.bmp", chess_board_src);

	Image chess_board_dst;
	make_image(chess_board_dst, chess_board_src.width, chess_board_src.height);

	x_gradients(chess_board_src, chess_board_dst);
	write_image(chess_board_dst, "x_gradients.bmp");

    y_gradients(chess_board_src, chess_board_dst);
	write_image(chess_board_dst, "y_gradients.bmp");

	dispose_image(chess_board_src);
	dispose_image(chess_board_dst);
}
```



```cpp
constexpr std::array<r32, 9> GRAD_TBLR_3X3
{
	-0.50f, -0.25f, 0.0f,
	-0.25f,  0.0f,  0.25f,
	 0.0f,   0.25f, 0.50f,
};
```

```cpp
constexpr std::array<r32, 9> GRAD_TBRL_3X3
{
	0.0f,  -0.25f, -0.50f,
	0.25f,  0.0f,  -0.25f,
	0.50f,  0.25f,  0.0f,
};
```

```cpp
#include <cmath>


r32 max_abs(r32 a, r32 b, r32 c, r32 d)
{
	auto max_ab = std::max(std::abs(a), std::abs(b));
	auto max_cd = std::max(std::abs(c), std::abs(d));

	return std::max(max_ab, max_cd);
}
```

```cpp
r32 gradient_at(Image const& img, u32 x, u32 y)
{
	r32 grad_lr = 0.0f;
	r32 grad_tb = 0.0f;
	r32 grad_tblr = 0.0f;
	r32 grad_tbrl = 0.0f;

	// x, y range
	auto x_begin = x - 1;
	auto x_end = x + 2;
	auto y_begin = y - 1;
	auto y_end = y + 2;

	u32 a = 0; // array index

	for (u32 v = y_begin; v < y_end; ++v)
	{
		for (u32 u = x_begin; u < x_end; ++u)
		{
			auto p = pixel_at(img, u, v);

			grad_lr += GRAD_LR_3X3[a] * p;
			grad_tb += GRAD_TB_3X3[a] * p;
			grad_tblr += GRAD_TBLR_3X3[a] * p;
			grad_tbrl += GRAD_TBRL_3X3[a] * p;
			++a;
		}
	}

	return max_abs(grad_lr, grad_tb, grad_tblr, grad_tbrl);
}
```

```cpp
void gradients(Image const& src, Image const& dst)
{
	zero_outer(dst);

	auto x_begin = 1;
	auto x_end = src.width - 1;
	auto y_begin = 1;
	auto y_end = src.height - 1;

	for (u32 y = y_begin; y < y_end; ++y)
	{
		auto dst_row = row_begin(dst, y);

		for (u32 x = x_begin; x < x_end; ++x)
		{
			auto grad = gradient_at(src, x, y);

			assert(grad >= 0.0f);
			assert(grad <= 255.0f);

			dst_row[x] = (u8)grad;
		}
	}
}
```

```cpp
int main()
{
    Image chess_board_src;
	read_image_from_file("chess_board.bmp", chess_board_src);

	Image chess_board_dst;
	make_image(chess_board_dst, chess_board_src.width, chess_board_src.height);

	x_gradients(chess_board_src, chess_board_dst);
	write_image(chess_board_dst, "x_gradients.bmp");

    y_gradients(chess_board_src, chess_board_dst);
	write_image(chess_board_dst, "y_gradients.bmp");

    gradients(chess_board_src, chess_board_dst);
	write_image(chess_board_dst, "gradients.bmp");

	dispose_image(chess_board_src);
	dispose_image(chess_board_dst);
}
```


```cpp
void edges(Image const& src, Image const& dst, u8 threshold)
{
	zero_outer(dst);

	for (u32 y = 1; y < src.height - 1; ++y)
	{
		auto dst_row = row_begin(dst, y);

		for (u32 x = 1; x < src.width - 1; ++x)
		{
			auto grad = gradient_at(src, x, y);

			assert(grad >= 0.0f);
			assert(grad <= 255.0f);

			dst_row[x] = (u8)grad > threshold ? 255 : 0;
		}
	}
}
```

```cpp
int main()
{
    Image chess_board_src;
	read_image_from_file("chess_board.bmp", chess_board_src);

	Image chess_board_dst;
	make_image(chess_board_dst, chess_board_src.width, chess_board_src.height);

	x_gradients(chess_board_src, chess_board_dst);
	write_image(chess_board_dst, "x_gradients.bmp");

    y_gradients(chess_board_src, chess_board_dst);
	write_image(chess_board_dst, "y_gradients.bmp");

    gradients(chess_board_src, chess_board_dst);
	write_image(chess_board_dst, "gradients.bmp");

    edges(chess_board_src, chess_board_dst, 25);
	write_image(chess_board_dst, "edges.bmp");

	dispose_image(chess_board_src);
	dispose_image(chess_board_dst);
}
```
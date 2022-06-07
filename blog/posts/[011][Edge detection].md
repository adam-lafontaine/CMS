# Edge detection
## Calculating image gradients

Images are digital representations of 2 dimensional pictures and don't have any information about the 3 dimensional world.  Inferring anything else from the image requires making some assumptions.  One reasonable assumption we can make is that different objects have different colors.  Therefore when pixels that are close to each other have different colors, they represent a boundary between two objects.  These boundary pixels are found by scanning an image looking for relatively large changes in color over small areas.  When a significant change is found at a pixel, we mark it as an edge.

Consider the folling image.

![alt text](https://github.com/adam-lafontaine/CMS/raw/p11-edge-detection/blog/img/%5B011%5D/orange_car.bmp)

After processing, we get a new image where edges are represented as white pixels on a black background.

![alt text](https://github.com/adam-lafontaine/CMS/raw/p11-edge-detection/blog/img/%5B011%5D/car_edges.bmp)

Although there is no color in the image, much of the information is preserved.  We can still tell that the image is of a car and its front dash and steering wheel.  Edge detection greatly simplifies an image while preserving important features making it a very useful tool for many image processing algorithms.

### Image gradients

Edge detection is done by first calculating the change in pixel intensities over the region surrounding each pixel.  The change in intensity is called the gradient.  If the gradient is high enough, the pixel is at an edge.  In this post we'll use grayscale images so that we only need to handle gradients for one channel.  Methods can be developed for color images but simply converting the image to grayscale is often good enough.

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


// https://almostalwaysauto.com/posts/what-is-an-image
void make_image(Image& image, u32 width, u32 height);

void dispose_image(Image& image);

// https://almostalwaysauto.com/posts/image-views
void read_image_from_file(const char* img_path_src, Image& image_dst);

void write_image(Image& image_src, const char* file_path_dst);
```

Image memory management is covered here: https://almostalwaysauto.com/posts/what-is-an-image.  Reading and writing grayscale images is covered at the end of this post: https://almostalwaysauto.com/posts/image-views.

When iterating over an image we'll need to find which row we're on for a given y coordinate.

```cpp
u8* row_begin(Image const& image, u32 y)
{
	auto row_offset = y * image.width;

	return image.data + row_offset;
}
```

The intesity value of a pixel is at the x offest of a given row.

```cpp
u8 pixel_at_xy(Image const& image, u32 x, u32 y)
{
	return row_begin(image, y)[x];
}
```

### Horizontal gradient

A horizontal gradient is calculated by finding the difference between the pixels to the right of a location and the pixels to the left of the location.  The following array will be used to calculate a gradient from left to right.

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

The array is arranged as a 3 x 3 matrix called a kernel to show how it will be used with each pixel.  The array and the pixels will be iterated over in such a way that the rows and columns corespond to the surrounding pixels at each position in the image.  Each value in the array is multiplied by the pixel value at that position.  The accumulated result gives a weighted difference between the pixels to the right and the pixels to the left.

We don't care if the result is positive or negative so we'll return the absolute value.

```cpp
r32 x_gradient_at_xy(Image const& img, u32 x, u32 y)
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
			auto p = pixel_at_xy(img, u, v);

			grad_lr += GRAD_LR_3X3[a] * p;
			++a;
		}
	}

	return std::abs(grad_lr);
}
```

The above function won't work for pixels on the outer edges of the image.  For instance, on the top row there are no pixels above it.  There are different methods for handling image boundaries.  The easiest is to simply set the gradient to zero.

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

We can now calculate the gradient for each pixel in an image and write the result to a new image.

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
			auto grad = x_gradient_at_xy(src, x, y);

			assert(grad >= 0.0f);
			assert(grad <= 255.0f);

			dst_row[x] = (u8)grad;
		}
	}
}
```

We'll use the following image and and the sample program below.

![alt text](https://github.com/adam-lafontaine/CMS/raw/p11-edge-detection/blog/img/%5B011%5D/chess_board.bmp)

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

The program...
* Reads the image from file
* Creates a new image to write the gradient values to
* Calculates the gradients
* Writes the gradient image to file

The image generated shows the vertical lines but not the horizontal ones.

![alt text](https://github.com/adam-lafontaine/CMS/raw/p11-edge-detection/blog/img/%5B011%5D/x_gradients.bmp)

### Vertical gradient

Calculating the vertical gradients is the identical approach except the values in the kernel are rotated so that the difference calculated will be from top to bottom.

```cpp
constexpr std::array<r32, 9> GRAD_TB_3X3
{
	-0.25f, -0.50f, -0.25f,
	 0.0f,   0.0f,   0.0f,
	 0.25f,  0.50f,  0.25f,
};
```

The logic for processing the image is exactly the same.  The only difference is that we use the new kernel.


```cpp
r32 y_gradient_at_xy(Image const& img, u32 x, u32 y)
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
			auto p = pixel_at_xy(img, u, v);

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
			auto grad = y_gradient_at_xy(src, x, y);

			assert(grad >= 0.0f);
			assert(grad <= 255.0f);

			dst_row[x] = (u8)grad;
		}
	}
}
```

Now we can update the program with the y_gradients function.

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

The output image shows the horizontal lines only.

![alt text](https://github.com/adam-lafontaine/CMS/raw/p11-edge-detection/blog/img/%5B011%5D/y_gradients.bmp)


### Custom gradient

Image gradients don't need to be only horizontal or vertical.  For instance, we can have a diagonal kernel that gives a gradient from top to bottom and left to right.

```cpp
constexpr std::array<r32, 9> GRAD_TBLR_3X3
{
	-0.50f, -0.25f, 0.0f,
	-0.25f,  0.0f,  0.25f,
	 0.0f,   0.25f, 0.50f,
};
```

We can also have top to bottom and right to left to get the gradient in the opposite direction.


```cpp
constexpr std::array<r32, 9> GRAD_TBRL_3X3
{
	0.0f,  -0.25f, -0.50f,
	0.25f,  0.0f,  -0.25f,
	0.50f,  0.25f,  0.0f,
};
```

We'll make a custom gradient algorithm that uses all four kernels and returns the maximum gradient found.  This helps account for edges along various angles, not just along the vertical and horizontal.

This helper function returns the maximum absolute value of four numbers.

```cpp
#include <cmath>


r32 max_abs(r32 a, r32 b, r32 c, r32 d)
{
	auto max_ab = std::max(std::abs(a), std::abs(b));
	auto max_cd = std::max(std::abs(c), std::abs(d));

	return std::max(max_ab, max_cd);
}
```

Calculate the gradient using the four kernels.

```cpp
r32 gradient_at_xy(Image const& img, u32 x, u32 y)
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
			auto p = pixel_at_xy(img, u, v);

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

Iterate over the image as before using the new algorithm.

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
			auto grad = gradient_at_xy(src, x, y);

			assert(grad >= 0.0f);
			assert(grad <= 255.0f);

			dst_row[x] = (u8)grad;
		}
	}
}
```

Update main.

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

The output shows the gradients for all directions.

![alt text](https://github.com/adam-lafontaine/CMS/raw/p11-edge-detection/blog/img/%5B011%5D/gradients.bmp)


### Edge detection

When the algorithm for generating image gradients is complete, most of the work for edge detection is done.  Basic edge detection requires one more step, which is to only show the gradients that are at or above a given threshold.  We can binarize each pixel after calculating its gradient.

```cpp
void edges(Image const& src, Image const& dst, u8 threshold)
{
	zero_outer(dst);

	for (u32 y = 1; y < src.height - 1; ++y)
	{
		auto dst_row = row_begin(dst, y);

		for (u32 x = 1; x < src.width - 1; ++x)
		{
			auto grad = gradient_at_xy(src, x, y);

			assert(grad >= 0.0f);
			assert(grad <= 255.0f);

			dst_row[x] = (u8)grad > threshold ? 255 : 0;
		}
	}
}
```

Add the edges function to main.

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

Only the significant edges are displayed and everything else is ignored.

![alt text](https://github.com/adam-lafontaine/CMS/raw/p11-edge-detection/blog/img/%5B011%5D/edges.bmp)


Notes
* light to dark or dark to light
* choose threshold
* choose gradient calculation
* blur image, preprocessing
* kernel(s)
* different kernel sizes, larger images
* negative/positive gradients
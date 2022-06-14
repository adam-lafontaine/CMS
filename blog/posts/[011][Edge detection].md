# Edge detection
## Calculating image gradients

Since images are digital representations of 2-dimensional pictures and don't have any information about the 3-dimensional world, inferring anything else requires making some assumptions.  One reasonable assumption we can make is that different objects have different colors.  Therefore when pixels that are close to each other have different colors they represent a boundary between two objects.  These boundary pixels are found by scanning an image and looking for relatively large changes in color over small regions.  When a significant change is found at a pixel, we mark it as being an edge.

Consider the following image.

![alt text](https://github.com/adam-lafontaine/CMS/raw/blurb/blog/img/%5B011%5D/orange_car.bmp)

When processing, we can generate a new image where the edges are represented as white pixels on a black background.

![alt text](https://github.com/adam-lafontaine/CMS/raw/current/blog/img/%5B011%5D/car_edges.bmp)

Although there is no color in the image, much of the information is preserved.  We can still tell that the image is of a car and its front dash and steering wheel.  Edge detection greatly simplifies an image while preserving important features.  This makes it a very useful tool for many image processing applications.

### Image gradients

Edge detection is done by first calculating the change in pixel intensities over the region surrounding each pixel.  The change in intensity is called the gradient.  If the gradient is high enough, the pixel is at an edge.

We'll use grayscale images so that we only need to deal with one channel.  Methods can be developed for color images but converting the image to grayscale is often good enough.

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

When iterating over an image we need to find which row we're on for a given y coordinate.

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

A horizontal gradient is calculated by finding the difference between the pixels to the right of a location and the pixels to the left of it.  The following array will be used to calculate a gradient from left to right.

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

The array is arranged as a 3 x 3 matrix called a kernel to show how it will be used with each pixel.  The array and the pixels will be iterated over in such a way that the rows and columns of the kernel correspond to the surrounding pixels at each position in the image.  Each value in the array is multiplied by the pixel value at that position.  The accumulated result gives a weighted difference between the pixels to the right and the pixels to the left.

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

The above function won't work for pixels on the outer rows and columns of the image.  For instance, on the top row there are no pixels above it.  There are different methods for handling image boundaries.  The easiest way is to simply set the gradient to zero.

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

Now we can calculate the gradient for each pixel in an image and write the result to a new image.  The pixel brightness in the output image will be the value of the calculated gradient.

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

We'll use the following image and the sample program below.

![alt text](https://github.com/adam-lafontaine/CMS/raw/current/blog/img/%5B011%5D/chess_board.bmp)

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

The image generated shows the vertical lines but not the horizontal ones.  Some of the lines are brigher than others.  The brighter pixels are those with the stronger gradients.

![alt text](https://github.com/adam-lafontaine/CMS/raw/current/blog/img/%5B011%5D/x_gradients.bmp)

### Vertical gradient

Calculating vertical gradients uses the identical approach except the values in the kernel are rotated so that the difference calculated will be from top to bottom.

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

This output image shows the horizontal lines only.

![alt text](https://github.com/adam-lafontaine/CMS/raw/current/blog/img/%5B011%5D/y_gradients.bmp)


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

We can also have top to bottom and right to left to get the gradient in the opposite diagonal direction.

```cpp
constexpr std::array<r32, 9> GRAD_TBRL_3X3
{
	0.0f,  -0.25f, -0.50f,
	0.25f,  0.0f,  -0.25f,
	0.50f,  0.25f,  0.0f,
};
```

We'll make a custom gradient algorithm that uses all four kernels and returns the maximum gradient found.  This helps account for edges along different angles.

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

![alt text](https://github.com/adam-lafontaine/CMS/raw/current/blog/img/%5B011%5D/gradients.bmp)


### Edge detection

When the algorithm for generating image gradients is complete, most of the work for edge detection is done already.  Basic edge detection requires only one more step, which is to only show the gradients that are at or above a given threshold.  To find an edge we can binarize each pixel after calculating its gradient.

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

			dst_row[x] = (u8)grad >= threshold ? 255 : 0;
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

    edges(chess_board_src, chess_board_dst, 26);
	write_image(chess_board_dst, "edges.bmp");

	dispose_image(chess_board_src);
	dispose_image(chess_board_dst);
}
```

Only the significant edges are displayed and everything else is ignored.

![alt text](https://github.com/adam-lafontaine/CMS/raw/current/blog/img/%5B011%5D/edges.bmp)


### Notes

There is no definitive process for finding edges in an image but the core principles remain the same.  Many decisions need to be made that will depend on the application.  For instance, the appropriate threshold depends on the brightness and contrast of the image.

Image gradients can be calculated in any number of ways.  Different kernels can be used having different values or dimensions.  For larger images with higher resolutions, larger kernels may be more appropriate if you need to look for edges accross a greater number of pixels.  Kernels do not need to be square either.  They can be rectangular or even 1-dimensional if you want to emphasize the gradient in one direction.

The horizontal and vertical gradients give us the orientation of an edge which allows for taking rotation into account when scanning for specific features.

Computers can't think or percieve the way humans can when shown an image.  They can only execute logic as they iterate over pixels.  Edge detection allows us to boil down the image to something simpler that still contains the information we want, but in a format that is more digestible for a software algorithm to process.
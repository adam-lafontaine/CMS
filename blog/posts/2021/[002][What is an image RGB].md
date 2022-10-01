# What is an image? - RGB(A) edition
## It's just another pointer


### RGB Images

Yes, a color image, just like a grayscale image, is simply a pointer to the image memory along with a width and a height.  However the image needs to contain red, green and blue data for each pixel instead of just one grayscale value.  Instead of having one 8 bit value, we can define a struct containing an 8 bit value for each color channel.

```cpp
#include <cstdint>

// make things easier to read
using u8 = uint8_t;    // 8 bit unsigned integer
using u32 = unsigned;  // 32 bit unsigned integer


typedef struct RGBPixel_t
{
	u8 red   = 0;
	u8 green = 0;
	u8 blue  = 0;

} RGBPixel;


typedef struct RGBImage_t
{
	u32 width  = 0;
	u32 height = 0;

	RGBPixel* data = nullptr;

} RGBImage;
```

The image struct holds the width and height and a pointer to the pixel data.

Each channel contains 8 bits, so they can have a value between 0 and 255.  Each combination of values is rendered as a different color.  When all three values are the same, the color is a shade of gray.  Black is when all three channels are 0 and white is when they are 255.


### RGBA Images

We can also have our image contain a fourth channel for the alpha value.  The alpha channel is used when we want to be able to store transparency.  It is useful for blending the image on top of another one.  The examples in this post will be done using RGBA images even though the alpha channel will not be used.

```cpp
typedef struct RGBAPixel_t
{
	u8 red   = 0;
	u8 green = 0;
	u8 blue  = 0;
	u8 alpha = 0;

} RGBAPixel;


typedef struct RGBAImage_t
{
	u32 width  = 0;
	u32 height = 0;

	RGBAPixel* data = nullptr;

} RGBAImage;


// use RGBA for our examples
using Pixel = RGBAPixel;
using Image = RGBAImage;
```


### Image Creation

Create and destroy images in the same fashion as grayscale images.

```cpp
#include <cassert>


void make_image(Image& image, u32 width, u32 height)
{
	assert(width);
	assert(height);

	image.width = width;
	image.height = height;
	image.data = (Pixel*)malloc(sizeof(Pixel) * width * height);

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


### Image Processing

Process each pixel.

```cpp
#include <functional>

void for_each_pixel(Image const& image, std::function<void(Pixel& p)> const& func)
{
    for(size_t i = 0; i < image.width * image.height; ++i)
    {
        func(image.data[i]);
    }
}
```

Process the pixels in terms of their x and y positions.

```cpp
void for_each_pixel(Image const& image, std::function<void(Pixel& p, u32 x, u32 y)> const& func)
{
    for(u32 y = 0; y < image.height; ++y)
    {
        auto row_offset = static_cast<size_t>(y * image.width);
        auto row_begin = image.data + row_offset;

        for(u32 x = 0; x < image.width; ++x)
        {
            func(row_begin[x], x, y);
        }
    }
}
```

Get the color of a pixel from its x and y position.

```cpp
Pixel pixel_value(Image const& image, u32 x, u32 y)
{
    auto row_offset = static_cast<size_t>(y * image.width);
    auto row_begin = image.data + row_offset;

    return row_begin[x];
}
```

For convenience, we can implement a comparison operator.

```cpp
bool operator == (Pixel const& lhs, Pixel const& rhs)
{
	return
		lhs.red == rhs.red &&
		lhs.green == rhs.green &&
		lhs.blue == rhs.blue &&
		lhs.alpha == rhs.alpha;
}
```


### Putting it all together

Below is a sample program to demonstrate how to generate images.

```cpp
#include <cassert>
#include <functional>
#include <cstdint>

// make things easier to read
using u8 = uint8_t;    // 8 bit unsigned integer
using u32 = unsigned;  // 32 bit unsigned integer


typedef struct RGBAPixel_t
{
	u8 red   = 0;
	u8 green = 0;
	u8 blue  = 0;
	u8 alpha = 0;

} RGBAPixel;


typedef struct RGBAImage_t
{
	u32 width  = 0;
	u32 height = 0;

	RGBAPixel* data = nullptr;

} RGBAImage;


using Pixel = RGBAPixel;
using Image = RGBAImage;


void make_image(Image& image, u32 width, u32 height)
{
	assert(width);
	assert(height);

	image.width = width;
	image.height = height;
	image.data = (Pixel*)malloc(sizeof(Pixel) * width * height);

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


void for_each_pixel(Image const& image, std::function<void(Pixel& p)> const& func)
{
	for (size_t i = 0; i < image.width * image.height; ++i)
	{
		func(image.data[i]);
	}
}


void for_each_pixel(Image const& image, std::function<void(Pixel& p, u32 x, u32 y)> const& func)
{
	for (u32 y = 0; y < image.height; ++y)
	{
		auto row_offset = static_cast<size_t>(y * image.width);
		auto row_begin = image.data + row_offset;

		for (u32 x = 0; x < image.width; ++x)
		{
			func(row_begin[x], x, y);
		}
	}
}


Pixel pixel_value(Image const& image, u32 x, u32 y)
{
	auto row_offset = static_cast<size_t>(y * image.width);
	auto row_begin = image.data + row_offset;

	return row_begin[x];
}


bool operator == (Pixel const& lhs, Pixel const& rhs)
{
	return
		lhs.red == rhs.red &&
		lhs.green == rhs.green &&
		lhs.blue == rhs.blue &&
		lhs.alpha == rhs.alpha;
}


void check_pixel(Pixel const& p, Pixel const& ex)
{
	auto const msg = p == ex ? "ok" : "not ok";
	printf_s("pixel: %2X %2X %2X %2X", p.red, p.green, p.blue, p.alpha);
	printf_s(", expected: %2X %2X %2X %2X", ex.red, ex.green, ex.blue, ex.alpha);
	printf_s(" - %s\n", msg);
}


int main()
{
	Pixel blue = { 0, 35, 139, 255 };
	Pixel white = { 255, 255, 255, 255 };
	Pixel red = { 237, 41, 57, 255 };
	Pixel black = { 0, 0, 0, 255 };
	Pixel gray = { 125, 125, 125, 255 };

	u32 image_width = 45;
	u32 image_height = 30;

	auto const french_flag = [&](Pixel& p, u32 x, u32 y)
	{
		if (x < image_width / 3) { p = blue; }
		else if (x < image_width * 2 / 3) { p = white; }
		else { p = red; }
	};

	auto const dutch_flag = [&](Pixel& p, u32 x, u32 y) 
	{
		if (y < image_height / 3) { p = red; }
		else if (y < image_height * 2 / 3) { p = white; }
		else { p = blue; }
	};

	auto const pattern = [&](Pixel& p, u32 x, u32 y)
	{
		if (x % 2 == 0 && y % 2 == 0) { p = black; }
		else if (x % 2 != 0 && y % 2 != 0) { p = gray; }
		else { p = white; }
	};

	Image image;	
	make_image(image, image_width, image_height);	

	for_each_pixel(image, french_flag);

	u32 blue_count = 0;
	auto const count_blue = [&](Pixel& p) { blue_count += (p == blue); };

	for_each_pixel(image, count_blue);

	std::cout << "\nFrench flag blue pixel count = " << blue_count << "\n";

	blue_count = 0;
	for_each_pixel(image, dutch_flag);
	for_each_pixel(image, count_blue);

	std::cout << "Dutch flag blue pixel count = " << blue_count << "\n\n";

	for_each_pixel(image, pattern);

	// check for black pixels (both even)
	check_pixel(pixel_value(image, 0, 0), black);
	check_pixel(pixel_value(image, 10, 6), black);

	// check for gray pixels (both odd)
	check_pixel(pixel_value(image, 5, 7), gray);
	check_pixel(pixel_value(image, 11, 1), gray);

	// check for white pixels (odd and even)
	check_pixel(pixel_value(image, 6, 3), white);
	check_pixel(pixel_value(image, 15, 8), white);

	dispose_image(image);
}
```
* The program makes an new image
* Colors the pixels either blue, white or red depending on the x position
* Counts the number of blue pixels to ensure that 1/3 of them are blue
* Colors the pixels based on their y positions
* Counts the blue pixels again to ensure that 1/3 of them are blue
* Writes over the image to generate the same black, gray and white pattern as the previous post
* Checks two pixels that should be black
* Checks two pixels that should be gray
* Checks two pixels that should be white
* Deallocates the image memory

The program output gives us what we expect.

![alt text](https://github.com/adam-lafontaine/CMS/raw/current/blog/img/%5B002%5D/output.bmp)

This is what the image first looked like after setting the color based on the x position.

![alt text](https://github.com/adam-lafontaine/CMS/raw/current/blog/img/%5B002%5D/french.bmp)

The image after setting the color based on the y position.

![alt text](https://github.com/adam-lafontaine/CMS/raw/current/blog/img/%5B002%5D/dutch.bmp)

The same checkerboard pattern as the previous post using RGB grayscale.

![alt text](https://github.com/adam-lafontaine/CMS/raw/current/blog/img/%5B002%5D/pattern.bmp)


### Conclusion

Handling color images is very similar to monochrome images.  It is more complex because there are either three or four different channels per pixel instead of just one byte.  However each channel is still one byte so an RGBA pixel is no different in memory than a 32 bit unsigned integer.  Packing the struct the way we do makes it easier to work with.

In the next post we'll see how we can handle image files.  That way you'll be able to see the images generated instead of taking my word for it with the screenshots provided.
# Reading and writing images
## Using stb_image


### stb

We will be using two public domain libraries called stb_image and stb_image_write.  the stb libraries (https://github.com/nothings/stb) is a collection of single-file public domain libraries provided by Sean T. Barrett and others.  They are easy to use because there is nothing to install and you only need to include the file(s) in your project.

### Reading image files
In order to read image files we need stb_image.

https://github.com/nothings/stb/blob/master/stb_image.h

The file has lots of documentation in the comments.  Below are some excerpts.

First it explains how to use the file.  There is an include guard around the implementation code so that we can include the API declarations in multiple files and have the implementation included only once.

```cpp
/* stb_image - v2.27 - public domain image loader - http://nothings.org/stb
                                  no warranty implied; use at your own risk
   Do this:
      #define STB_IMAGE_IMPLEMENTATION
   before you include this file in *one* C or C++ file to create the implementation.

   // i.e. it should look like this:
   #include ...
   #include ...
   #include ...
   #define STB_IMAGE_IMPLEMENTATION
   #include "stb_image.h"

   ...

*/
```

Next, there is an explanation of how the API works.  You specify the number of channels you want your image data to have and pass variable references for the width and height.  The function returns a pointer to the image data.

```cpp
// Standard parameters:
//    int *x                 -- outputs image width in pixels
//    int *y                 -- outputs image height in pixels
//    int *channels_in_file  -- outputs # of image components in image file
//    int desired_channels   -- if non-zero, # of image components requested in result
//
// The return value from an image loader is an 'unsigned char *' which points
// to the pixel data, or NULL on an allocation failure or if the image is
// corrupt or invalid. The pixel data consists of *y scanlines of *x pixels,
// with each pixel consisting of N interleaved 8-bit components; the first
// pixel pointed to is top-left-most in the image. There is no padding between
// image scanlines or between pixels, regardless of format. The number of
// components N is 'desired_channels' if desired_channels is non-zero, or
// *channels_in_file otherwise. If desired_channels is non-zero,
// *channels_in_file has the number of components that _would_ have been
// output otherwise. E.g. if you set desired_channels to 4, you will always
// get RGBA output, but you can check *channels_in_file to see if it's trivially
// opaque because e.g. there were only 3 channels in the source image.
//
// An output image with N components has the following components interleaved
// in this order in each pixel:
//
//     N=#comp     components
//       1           grey
//       2           grey, alpha
//       3           red, green, blue
//       4           red, green, blue, alpha
```

The function we will use is below.  It returns a pointer to an unsigned char which is a one byte unsigned integer.

```cpp
typedef unsigned char stbi_uc;
#define STBIDEF extern

STBIDEF stbi_uc *stbi_load(char const *filename, int *x, int *y, int *channels_in_file, int desired_channels);
```

Using what we've done in the previous posts, we can wrap the function for our use case.

```cpp
#include <cassert>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


using u8 = unsigned char;
using u32 = unsigned;


typedef struct RGBAPixel_t
{
	u8 red   = 0;
	u8 green = 0;
	u8 blue  = 0;
	u8 alpha = 0;

} Pixel;


typedef struct RGBAImage_t
{
	u32 width  = 0;
	u32 height = 0;

	Pixel* data = nullptr;

} Image;


void read_image_from_file(const char* img_path_src, Image& image_dst)
{
    int width = 0;
    int height = 0;
    int image_channels = 0;
    int desired_channels = 4;

    auto data = (Pixel*)stbi_load(img_path_src, &width, &height, &image_channels, desired_channels);

    assert(data);
    assert(width);
    assert(height);

    image_dst.data = data;
    image_dst.width = width;
    image_dst.height = height;
}
```

The 4 channels are interleaved, meaning that each pixel is 4 consecutive bytes.  Casting the pointer returned as a Pixel* will allow us to iterate over the data 4 bytes at a time.

### Writing images

The code needed to write image files is contained in another library called stb_image_write.

https://github.com/nothings/stb/blob/master/stb_image_write.h

It has a similar include guard.

```cpp
/* stb_image_write - v1.16 - public domain - http://nothings.org/stb
   writes out PNG/BMP/TGA/JPEG/HDR images to C stdio - Sean Barrett 2010-2015
                                     no warranty implied; use at your own risk

   Before #including,

      #define STB_IMAGE_WRITE_IMPLEMENTATION

   in the file that you want to have the implementation.

   ...

   */
```

There are functions for writing multiple file types.

```cpp
/*
...

USAGE:

   There are five functions, one for each image file format:

     int stbi_write_png(char const *filename, int w, int h, int comp, const void *data, int stride_in_bytes);
     int stbi_write_bmp(char const *filename, int w, int h, int comp, const void *data);
     int stbi_write_tga(char const *filename, int w, int h, int comp, const void *data);
     int stbi_write_jpg(char const *filename, int w, int h, int comp, const void *data, int quality);
     int stbi_write_hdr(char const *filename, int w, int h, int comp, const float *data);

...
*/
```

We will use the bitmap (bmp) version for our example.

```cpp
#define STBIWDEF  extern

STBIWDEF int stbi_write_bmp(char const *filename, int w, int h, int comp, const void  *data);
```

Here is a function we can use to write one our images.

```cpp
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

void write_image(Image const& image_src, const char* file_path_dst)
{
    assert(image_src.width);
    assert(image_src.height);
    assert(image_src.data);

    int width = static_cast<int>(image_src.width);
    int height = static_cast<int>(image_src.height);
    int channels = 4;
    auto const data = image_src.data;

    int result = 0;

    result = stbi_write_bmp(file_path_dst, width, height, channels, data);

    assert(result);
}
```

### Declarations

In order to demonstrate, we first need to get organized.  Create a header file called image.hpp and paste the following code in it.  Here we define our pixel and image and declare the functions we will use.

```cpp
#pragma once

#include <functional>

using u8 = unsigned char;
using u32 = unsigned;


typedef struct RGBAPixel_t
{
    u8 red = 0;
    u8 green = 0;
    u8 blue = 0;
    u8 alpha = 0;

} Pixel;


typedef struct RGBAImage_t
{
    u32 width = 0;
    u32 height = 0;

    Pixel* data = nullptr;

} Image;


bool operator == (Pixel const& lhs, Pixel const& rhs);


void dispose_image(Image& image);

void for_each_pixel(Image const& image, std::function<void(Pixel& p)> const& func);

Pixel pixel_value(Image const& image, u32 x, u32 y);


void read_image_from_file(const char* img_path_src, Image& image_dst);

void write_image(Image const& image_src, const char* file_path_dst);
```

### Implementation

Next, create a file called image.cpp for the implementation of the above declarations.

```cpp
#include "image.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <cassert>


bool operator == (Pixel const& lhs, Pixel const& rhs)
{
    return
        lhs.red == rhs.red &&
        lhs.green == rhs.green &&
        lhs.blue == rhs.blue &&
        lhs.alpha == rhs.alpha;
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


Pixel pixel_value(Image const& image, u32 x, u32 y)
{
    auto row_offset = static_cast<size_t>(y * image.width);
    auto row_begin = image.data + row_offset;

    return row_begin[x];
}


void read_image_from_file(const char* img_path_src, Image& image_dst)
{
    int width = 0;
    int height = 0;
    int image_channels = 0;
    int desired_channels = 4;

    auto data = (Pixel*)stbi_load(img_path_src, &width, &height, &image_channels, desired_channels);

    assert(data);
    assert(width);
    assert(height);

    image_dst.data = data;
    image_dst.width = width;
    image_dst.height = height;
}


void write_image(Image const& image_src, const char* file_path_dst)
{
    assert(image_src.width);
    assert(image_src.height);
    assert(image_src.data);

    int width = static_cast<int>(image_src.width);
    int height = static_cast<int>(image_src.height);
    int channels = 4;
    auto const data = image_src.data;

    int result = 0;

    result = stbi_write_bmp(file_path_dst, width, height, channels, data);

    assert(result);
}
```

### Sample Program

I will use the following image for our demonstration.  The code is below.

![alt text](https://github.com/adam-lafontaine/CMS/raw/master/img/%5B003%5D/pixel-character.png)

```cpp

#include "image.hpp"

int main()
{
    auto image_src_path = "pixel-character.png";

	Image image;
	read_image_from_file(image_src_path, image);

	Pixel blue = { 0, 35, 139, 255 };
	Pixel red = { 237, 41, 57, 255 };
	Pixel white = { 255, 255, 255, 255 };

    // find the color of the character's shirt by selecting the pixel in the middle of the image
	auto search_color = pixel_value(image, image.width / 2, image.height / 2);
	
    // change the character's shirt to blue
    auto dst_color = blue;	

	auto const convert = [&](Pixel& p) 
	{
		if (p == search_color)
		{
			p = dst_color;
		}
	};

	for_each_pixel(image, convert);

	write_image(image, "pixel-character-blue.bmp");

    // now change the blue shirt to red
	search_color = blue;
	dst_color = red;

	for_each_pixel(image, convert);

	write_image(image, "pixel_character-red.bmp");

	dispose_image(image);
}
```

Program summary:
* Read the png image
* Get the image's center pixel to determine the color of the character's shirt
* Set each pixel of the character's shirt to blue
* Write the image as a bmp
* Set each pixel of the character's shirt to red
* Write another image as a bmp
* Cleanup the image memory

Here are the two images generated by the program.

![alt text](https://github.com/adam-lafontaine/CMS/raw/master/img/%5B003%5D/pixel-character-blue.bmp)

![alt text](https://github.com/adam-lafontaine/CMS/raw/master/img/%5B003%5D/pixel-character-red.bmp)

### Thank you stb

Thanks to Sean Barrett and the other contributors to the stb libraries, normies like you and I can just get work done.  We can read any image type and write to any other image type allowing us to process all images in the same manner regardless of file format.  We can also create our own images and save them as any file type.  This saves us a lot of time trying to understand the specifications of each image file format.  To get an idea what this would take, go over the source code in stb_image.h and stb_image_write.h.  Thankfully we can just take it for granted.
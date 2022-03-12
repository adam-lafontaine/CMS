# What is an image?
## It's acually pretty easy

I'm pretty old, so I remember when dealing with images on a computer was rare.  Normally you would take your film cartrige to the store to get your pictures developed.  If you wanted your pictures right away, you would need to have a Polaroid.  This of course was back in the day when schools where placed geographically such that you would have to walk uphill to get there and then uphill again to return home.  How this was managed is a secret that my parents' generation have kept to this day.  But such conspiracies are a topic for a different website.

So what is a digital image?  It's not a 2D matrix, although you can treat is as one if you like.  It is a chunk of memory on your hard drive, or in RAM if you are actually looking at it.  Actually, the same can be said for anything on your computer.  The real question is how do we deal with it in code?

### Monochrome Images

An image is simply a pointer with a width and height.  We can represent it using a simple struct.

```cpp
#include <cstdint>

// make things easier to read
using u8 = uint8_t;    // 8 bit unsigned integer
using u32 = unsigned;  // 32 bit unsigned integer


typedef struct MonoImage_t
{
    u32 width  = 0;
    u32 height = 0;
    u8* data   = nullptr;

} MonoImage;
```

This is a monochrome (grayscale) image.  Each pixel is represented as an 8 bit unsigned integer with values ranging from 0 to 255.  A value of 0 means that the pixel is black, and 255 means that the pixel is white.  The values in between are different shades of gray.


### Image Creation

The pixels themselves are a block of memory with a size in bytes equal to the product of the width and height.  Each pixel being one byte.  The pointer in our struct is the address of the first byte in the block of memory.  In order to create a new image, we simply need to allocate memory on the heap.

```cpp
#include <cassert>

void make_image(MonoImage& image, u32 width, u32 height)
{
    assert(width);
    assert(height);

    image.width = width;
    image.height = height;
    image.data = (u8*)malloc(sizeof(u8) * width * height);

    assert(image.data);
}
```

We will also need to clean up after ourselves when we are done with the image.

```cpp
void dispose_image(MonoImage& image)
{
    if (image.data != nullptr)
    {
        free(image.data);
        image.data = nullptr;
    }
}
```


### Image Processing

Iterating over an image is pretty straightforward.

```cpp
void process_pixel(u8& p)
{
    // do something with p
}

auto size = image.width * image.height;

for(size_t i = 0; i < size; ++i)
{
    process_pixel(image.data[i]);
}
```

In more general terms...

```cpp
#include <functional>

void for_each_pixel(MonoImage const& image, std::function<void(u8& p)> const& func)
{
    for(size_t i = 0; i < image.width * image.height; ++i)
    {
        func(image.data[i]);
    }
}
```

How an image is represented and rendered depends on the file type (e.g. bmp, jpg, png) and the operating system.  For the purposes of this article, the image data begins at the top left corner of the image and each subsequent pixel is the one to the right of it.  When the width is reached, the pixel is the beginning of the next row.

We can also iterate over an image in terms of its x and y coodinates.

```cpp
for(u32 y = 0; y < image.height; ++y)
{
    // each y value is another row
    // its distance from the beginning is the y value times the width
    auto row_offset = static_cast<size_t>(y * image.width);
    
    // pointer to the beginning of the current row
    auto row_begin = image.data + row_offset;

    for(u32 x = 0; x < image.width; ++x)
    {
        process_pixel(row_begin[x]);
    }
}
```

Again, in more general terms.

```cpp
void for_each_pixel(MonoImage const& image, std::function<void(u8& p, u32 x, u32 y)> const& func)
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

Using what we know from above, we can get the value of a pixel at a given x and y.

```cpp
u8 pixel_value(MonoImage const& image, u32 x, u32 y)
{
    auto row_offset = static_cast<size_t>(y * image.width);
    auto row_begin = image.data + row_offset;

    return row_begin[x];
}
```

### Putting it all together

Sample program below.

```cpp
#include <cstdint>
#include <cassert>
#include <functional>

using u8 = uint8_t;
using u32 = unsigned;

typedef struct MonoImage_t
{
    u32 width  = 0;
    u32 height = 0;
    u8* data   = nullptr;

} MonoImage;


void make_image(MonoImage& image, u32 width, u32 height)
{
    assert(width);
    assert(height);

    image.width = width;
    image.height = height;
    image.data = (u8*)malloc(sizeof(u8) * width * height);

    assert(image.data);
}


void dispose_image(MonoImage& image)
{
    if (image.data != nullptr)
    {
        free(image.data);
        image.data = nullptr;
    }
}


void for_each_pixel(MonoImage const& image, std::function<void(u8& p)> const& func)
{
    for(size_t i = 0; i < image.width * image.height; ++i)
    {
        func(image.data[i]);
    }
}


void for_each_pixel(MonoImage const& image, std::function<void(u8& p, u32 x, u32 y)> const& func)
{
    for(u32 y = 0; y < image.height; ++y)
    {
        auto row_offset = static_cast<size_t>(y * image.width);
        auto row_begin = image.data + row_offset;

        for(u32 x = 0; x < image.width; ++x)
        {
            auto& p = row_begin[x];
            func(p, x, y);
        }
    }
}


u8 pixel_value(MonoImage const& image, u32 x, u32 y)
{
    auto row_offset = static_cast<size_t>(y * image.width);
    auto row_begin = image.data + row_offset;

    return row_begin[x];
}


int main()
{
    // lambda to set a pixel to white
    auto const set_white = [](u8& p) { p = 255; };

    // lambda to set a pixel to black if x and y are both even
    auto const black_if_even = [](u8& p, u32 x, u32 y)
    {
        if (x % 2 == 0 && y % 2 == 0) { p = 0; }
    };

    // lambda to set a pixel to gray if x and y are both odd
    auto const gray_if_odd = [](u8& p, u32 x, u32 y)
    {
        if (x % 2 != 0 && y % 2 != 0) { p = 125; }
    };

    // print to console whether a pixel is what we think it should be
    auto const check_pixel = [](u8 p, u8 expected_value) 
    {
        auto const msg = (p == expected_value) ? "ok" : "not ok";
        std::cout << "pixel: " << u32(p) << ", expected: " << u32(expected_value) << " - " << msg << "\n";
    };


    MonoImage image;
    u32 image_width = 16; // 16 pixels wide
    u32 image_height = 9; // 9 pixels high

    make_image(image, image_width, image_height);    

    // set all pixels to white
    for_each_pixel(image, set_white);

    // make sure pixels are white
    check_pixel(pixel_value(image, 0, 0), 255);
    check_pixel(pixel_value(image, 3, 3), 255);
    check_pixel(pixel_value(image, 15, 8), 255);
    
    // set even x and y pixels to black
    for_each_pixel(image, black_if_even);

    // set odd x and y pixels to gray
    for_each_pixel(image, gray_if_odd);

    // check for black pixels (both even)
    check_pixel(pixel_value(image, 0, 0), 0);
    check_pixel(pixel_value(image, 10, 6), 0);

    // check for gray pixels (both odd)
    check_pixel(pixel_value(image, 5, 7), 125);
    check_pixel(pixel_value(image, 11, 1), 125);

    // check for white pixels (odd and even)
    check_pixel(pixel_value(image, 6, 3), 255);
    check_pixel(pixel_value(image, 15, 8), 255);

    dispose_image(image);
}
```

* The program first creates a new image
* Each pixel is set to the value of 255, i.e. white
* Check three different x and y positions to make sure that they are in fact white
* Set pixels to black (0) if both the x and y values are even numbers
* Set pixels to gray (125) if both the x and y values are odd numbers
* Check two x and y postions that should be black
* Check two x and y positions that should be gray
* Check two x and y positions that should still be white
* Finally, release the image memory

The output from the program shows that the pixels we checked were what we expected them to be.

![alt text](https://github.com/adam-lafontaine/CMS/raw/current/blog/img/%5B001%5D/output.bmp)

The image generated by the program was only 16 pixels wide by 9 pixels high.  Here is an enlarged version.

![alt text](https://github.com/adam-lafontaine/CMS/raw/current/blog/img/%5B001%5D/bw.bmp)

I told you it was easy.  Without very much work, we can create a black and white image and manipulate it however we want.  We simply assign a value from 0 to 255 to a byte in memory and be sure that our coordinate system matches that of how we read and write images.  Reading and writing will be covered in a future post, but we will first need to discuss how to handle color images.
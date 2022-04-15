s# Simple Image Transforms
## Grayscale, Binarization

### The standard algorithms

In a [previous post](https://almostalwaysauto.com/posts/parallelism-for-free) I wrote about the C++ Standard Template Library (STL) and demonstrated some of the algorithms that are available.  These algorithms are not only available for standard containers such as std::vector and std::array, but they can be used for any container with a valid iterator.  How to write a valid iterator is another topic and doing it is kind of irritating.  It is basically an object that behaves like a pointer to an item in the container.  The idea is that when we create our custom containers, we can have access to the STL algorthms at the cost of implementing some C++ bureaucracy.  When the iterator of your container is a raw pointer, the bureaucracy is minimal.

In this post, our containers are images and the items in our containers are pixels.

```cpp
using u32 = unsigned;      // 4 bytes
using u8 = unsigned char;  // 1 byte


class RGBAPixel
{
public:
    u8 red;
    u8 green;
    u8 blue;
    u8 alpha;
};


class RGBAImage
{
public:
    u32 width;
    u32 height;

    RGBAPixel* data;
};


class GrayImage
{
public:
    u32 width;
    u32 height;

    u8* data;
};
```

The algorithms require a range of elements to iterate over.  The standard containters provide functions called begin() and end().  begin() points to the first item in the container and end() points to one past the last item.  So if we want to use the STL algorithms on our images, all we need to do is implement begin() and end() functions.  The first item is pointed to by the data pointer.  One past the last item will be the number of pixels (i.e. width x height) after the first.  Our image classes become the following.

```cpp
class RGBAImage
{
public:
    u32 width;
    u32 height;

    RGBAPixel* data;

    RGBAPixel* begin() { return data; }
    RGBAPixel* end() { return data + (size_t)width * (size_t)height; }
};


class GrayImage
{
public:
    u32 width;
    u32 height;

    u8* data;

    u8* begin() { return data; }
    u8* end() { return data + (size_t)width * (size_t)height; }
};
```

And that is it.  The static casts are there to prevent compliler warnings.  Now we can do things like:

```cpp
// make all pixels white
std::fill(gray_image.begin(), gray_image.end(), 255);
```

### Grayscale

Converting a color image to black and white is done simply by iterating over each pixel and taking a weighted average of the red, green and blue channels.  Here is an implementation of the formula that is typically used.

```cpp
u8 to_grayscale(u8 red, u8 green, u8 blue)
{
    return (u8)(0.299 * red + 0.587 * green + 0.114 * blue);
}
```

To convert an image to grayscale, we would use the std::transform algorithm.

```cpp
#include <cassert>
#include <algorithm>


void convert_to_grayscale(RGBAImage& src, GrayImage& dst)
{
    assert(src.data);
    assert(dst.data);
    assert(src.width == dst.width);
    assert(src.height == dst.height);

    auto const convert = [](RGBAPixel const& p) { return to_grayscale(p.red, p.green, p.blue); };

    std::transform(src.begin(), src.end(), dst.begin(), convert);
}
```

Starting with an image containing two sprites.

![alt text](https://github.com/adam-lafontaine/CMS/raw/current/blog/img/%5B006%5D/sprites.png)

We get the following grayscale image

![alt text](https://github.com/adam-lafontaine/CMS/raw/current/blog/img/%5B006%5D/sprites_gray.bmp)


### Binarization

Sometimes we want to set each pixel in an image to one of two colors (usually black or white) depending on certain conditions.  The simplest case is to take a grayscale image and evaluate each pixel to see if it falls above a given threshold.  Where it is in relation to the threshold determies the color of the pixel in the output image.

```cpp
void binarize_simple(GrayImage& src, GrayImage& dst)
{
    assert(src.data);
    assert(dst.data);
    assert(src.width == dst.width);
    assert(src.height == dst.height);

    auto const convert = [](u8 p) { return p > 150 ? 255 : 0; };

    std::transform(src.begin(), src.end(), dst.begin(), convert);
}
```

If we don't care about preserving the original image data, we can binarize it without having to create a new image in memory.

```cpp
void binarize_simple(GrayImage& src_dst)
{
    assert(src_dst.data);

    auto const convert = [](u8& p) { p = p > 150 ? 255 : 0; };

    std::for_each(src_dst.begin(), src_dst.end(), convert);
}
```

Applying the function with the grayscale image we just generated gives the following.

![alt text](https://github.com/adam-lafontaine/CMS/raw/current/blog/img/%5B006%5D/binary.bmp)


### Example Program

Below is a program demonstrating what was covered.  There are a few functions that were implemented in previous posts that I'm not repeating here.  Technically only color images were covered for writing images, but figuring it out for grayscale images shouldn't be too difficult.

```cpp
// https://almostalwaysauto.com/posts/read-write-image
void read_image_from_file(const char* img_path_src, RGBAImage& image_dst);
void write_image(GrayImage const& image_src, const char* file_path_dst); // hint: channels = 1

// https://almostalwaysauto.com/posts/what-is-an-image-rgb
void dispose_image(RGBAImage& image);

// https://almostalwaysauto.com/posts/what-is-an-image
void make_image(GrayImage& image, u32 width, u32 height);
void dispose_image(GrayImage& image);


auto constexpr sprites_src = "sprites.png";
auto constexpr sprites_gray_dst = "sprites_gray.bmp";
auto constexpr binary_dst = "binary.bmp";


int main()
{
    RGBAImage rgba;
    read_image_from_file(sprites_src, rgba);

    auto width = rgba.width;
    auto height = rgba.height;

    GrayImage gray;
    make_image(gray, width, height);
    convert_to_grayscale(rgba, gray);
    write_image(gray, sprites_gray_dst);

    binarize_simple(gray);
    write_image(gray, binary_dst);

    dispose_image(rgba);
    dispose_image(gray);
}
```

The standard algorithms library is a handy set of tools to have.  Getting access to it requires providing the alorithms with a means of iterating over your collection of elements.  Implementing an iterator can be prohibitive but if your data simply resides in a buffer, then it's quite easy.  The algorithms just work, they make your code more readable and are perfect for when you don't want to reinvent the wheel.  In some situations you will need to reinvent the wheel and a standard algorithm may not be suitable.  In fact, many companies have their own implementations of a standard library that works best for their technology.  Whatevever your situation, they are an excellent resource to have and worth becoming familiar with.
# Image Views
## 


### The image class

```cpp
#include <cstdint>

using u32 = uint32_t;
using u8 = uint8_t;

using GrayPixel = u8;


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

Add a constructor and destructor

```cpp
#include <cstdlib>


class GrayImage
{
public:
    u32 width = 0;
    u32 height = 0;

    u8* data = nullptr;

    u8* begin() { return data; }
    u8* end() { return data + (size_t)width * (size_t)height; }

    u8* begin() const { return data; }
    u8* end() const { return data + (size_t)width * (size_t)height; }


    GrayImage() {};


    GrayImage(u32 w, u32 h)
    {
        auto const bytes = w * h * sizeof(u8);
        auto ptr = (u8*)malloc(bytes);
        if (ptr)
        {
            data = ptr;
            width = width;
            height = height;
        }
    }


    ~GrayImage()
    {
        if (data)
        {
            free(data);
            data = nullptr;
        }
    }
};
```

### Objective

Suppose we have functions for reading and writing images.  This was covered in a previous post.

```cpp
// https://almostalwaysauto.com/posts/read-write-image

void read_image_from_file(const char* img_path_src, GrayImage& image_dst);

void write_image(GrayImage const& image_src, const char* file_path_dst);
```

We want to be able to create a view from an existing image and range of pixel locations.

```cpp
class Range2Du32
{
public:
    u32 x_begin;
    u32 x_end;
    u32 y_begin;
    u32 y_end;
};


class GrayView
{
public:

    GrayView(GrayImage const& image, Range2Du32 const& range);

    // TODO: implement
};
```

For our example, we'll a function that uses a GrayView to operate its pixels.

```cpp
// a function for operating on the selected region
void invert_gray(GrayView const& view); // TODO: implement
```

The goal is to be able to use the GrayView class like so:

```cpp
int main()
{
    // get an image from file
    GrayImage image;
    read_image_from_file("image path", image);
    auto const width = image.width;
    auto const height = image.height;

    // generate a view of the center region of the image
    Range2Du32 range{};
    range.x_begin = width / 4;
    range.x_end = width * 3 / 4;
    range.y_begin = height / 4;
    range.y_end = height * 3 / 4;

    GrayView view(image, range);

    // invert the grayscale colors of the pixels within the view
    invert_gray(view);

    // write the modified image to file
    write_image(image, "new image path");
}
```


```cpp
#include <algorithm>


void invert_gray(GrayView const& view)
{
    auto const invert = [](GrayPixel& p) { p = 255 - p; };

    std::for_each(view.begin(), view.end(), invert);
}
```

From this we can see that in addition to implementing the constructor, we need begin and end iterator functions if we want to use the standard algorithms.

### the view class

Given our requirements, the GrayView class needs to have the following structure.

```cpp
class GrayView
{
public:    

    GrayView(GrayImage const& image, Range2Du32 const& range); // TODO: implement


    class iterator; // TODO: implement

    iterator begin(); // TODO: implement

    iterator end(); // TODO: implement

    iterator begin() const; // TODO: implement

    iterator end() const; // TODO: implement
};
```

Before we handle the iterator, we need to specify what the view object needs to be able to iterate over a range of an image.  i.e. the image's properties as well as the range.  Adding these properties makes implementing the constructor trivial.

```cpp
class GrayView
{
public:

    GrayPixel* image_data_ = nullptr;
    u32 image_width_ = 0;
    u32 image_height_ = 0;

    GrayView(GrayImage const& image, Range2Du32 const& range)
    {
        image_data_ = image.data;
        image_width_ = image.width;
        image_height_ = image.height;
        range_ = range;
    }


    class iterator; // TODO: implement

    iterator begin(); // TODO: implement

    iterator end(); // TODO: implement

    iterator begin() const; // TODO: implement

    iterator end() const; // TODO: implement
};
```

### The iterator class

The iterator for the GrayView class needs to be a forward iterator.  That means at minimum it must be able to increment (++), compare equality (==, !=) and dereference(*).  We'll start with the following definitions.

```cpp
class GrayView::iterator
{
public:

    using iterator_category = std::forward_iterator_tag;
    using value_type = GrayPixel;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type*;
    using reference = value_type&;

    explicit iterator() {}

    explicit iterator(GrayView const& view); // TODO: implement

    iterator& operator ++ (); // TODO: implement

    iterator operator ++ (int); // TODO: implement

    bool operator == (iterator other) const; // TODO: implement

    bool operator != (iterator other) const; // TODO: implement

    reference operator * () const; // TODO: implement
};
```

When the iterator is created from a view, it will set its location to the beginning of the range.  It will also need to store the location of the image memory and the range of pixel locations represented by the view.

```cpp
class GrayView::iterator
{
public:

    using iterator_category = std::forward_iterator_tag;
    using value_type = GrayPixel;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type*;
    using reference = value_type&;

    // location within the image
    u32 x_ = 0;
    u32 y_ = 0;

    GrayPixel* image_data_ = nullptr;
    Range2Du32 range_ = {};

    explicit iterator() {}

    explicit iterator(GrayView const& view)
    {
        image_data_ = view.image_data_;
        range_ = view.range_;
        
        x_ = range_.x_begin;
        y_ = range_.y_begin;
    }

    iterator& operator ++ ();

    iterator operator ++ (int);

    bool operator == (iterator other) const;

    bool operator != (iterator other) const;

    reference operator * () const;
};
```

Increment

```cpp

```

Equality

```cpp

```

Dereference

```cpp

```
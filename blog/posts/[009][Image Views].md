# Image Views
## Working on a range or region of pixels within an image

### What is a view?

Views can be described as "lightweight objects that indirectly represent iterable sequences" (https://en.cppreference.com/w/cpp/ranges).  

In image processing, there is a section of the image called the Region of Interest (ROI).  The ROI is the region that contains the features we are interested in and is where we want to apply our algorithm.  Having a view to the ROI

I first learned about image views from the Boost GIL library (https://www.boost.org/doc/libs/1_77_0/libs/gil/doc/html/design/image_view.html)

> An image view is a generalization of STL range concept to multiple dimensions. Similar to ranges (and iterators), image views are shallow, don’t own the underlying data and don’t propagate their constness over the data. For example, a constant image view cannot be resized, but may allow modifying the pixels

For example, suppose we want to select a range of elements within a vector and treat that range as a collection itself.  We want to be able to access and modify each element in the range and see the changes we made when reading the original vector.  For this, we could make something like the following.

```cpp
#include <vector>

using VecIter = std::vector<int>::iterator;

class VectorView
{
private:

    size_t begin_;
    size_t end_;

    std::vector<int>* vec_;

public:    

    VectorView(std::vector<int>& v, size_t begin, size_t end)
    {
        vec_ = &v;
        begin_ = begin;
        end_ = end;
    }

    VecIter begin() { return (*vec_).begin() + begin_; }

    VecIter end() { return (*vec_).begin() + end_; }

    VecIter begin() const { return (*vec_).begin() + begin_; }

    VecIter end() const { return (*vec_).begin() + end_; }
};
```

This VectorView has a vector (pointer) and the offsets that define the range of elements it acts on.  The begin() and end() methods provide the bureaucracy needed to use the STL algorithms and other standard C++ functionality.

Here's a small program to demonstrate how it would work.

```cpp
#include <cstdlib>


int main()
{
    // function to display a collection in the console
    auto const print_vec = [](auto const& vec, const char* label)
    {
        printf("\n%s", label);
        printf("\n{ ");
        for (auto const& item : vec) { printf("%d ", item); }
        printf("}\n");
    };

    // create a vector
    std::vector<int> vec{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    print_vec(vec, "vector before:");

    // create a view
    VectorView view(vec, 2, 7);
    print_vec(view, "view before:");

    // modify the elements of the view
    for (auto& item : view)
    {
        item *= -1;
    }

    // display the results
    print_vec(view, "view after:");
    print_vec(vec, "vector after:");
}
```

Output

```
vector before:
{ 1 2 3 4 5 6 7 8 9 }

view before:
{ 3 4 5 6 7 }

view after:
{ -3 -4 -5 -6 -7 }

vector after:
{ 1 2 -3 -4 -5 -6 -7 8 9 }

new vector:
{ -3 -4 -5 -6 -7 }
```



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

    u8* begin() const { return data; }
    u8* end() const { return data + (size_t)width * (size_t)height; }
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
            width = w;
            height = h;
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

Suppose we have functions for reading and writing images.  This was covered in a previous post for color images.  See the end of this post for reading grayscale images.

```cpp
// https://almostalwaysauto.com/posts/read-write-image

void read_image_from_file(const char* img_path_src, GrayImage& image_dst);

void write_image(GrayImage const& image_src, const char* file_path_dst);
```

We want to be able to create a view from an existing image using a 2 dimensional range of pixel locations.  The view should point to the range of pixels and any operations performed on the view should only affect the pixels in the range.  We'll first define a range of pixels within an image.

```cpp
class Range2Du32
{
public:
    u32 x_begin;
    u32 x_end;
    u32 y_begin;
    u32 y_end;
};
```

For our example, we'll define a function that uses a GrayView to operate on an image's pixels.  We'll make make the view behave like a standard container so that we can use it with the standard algorithms.

```cpp
#include <algorithm>


// a function for operating on the selected region
void invert_gray(GrayView const& view)
{
    auto const invert = [](GrayPixel& p) { p = 255 - p; };

    std::for_each(view.begin(), view.end(), invert);
}
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

    // create a view
    Range2Du32 range{};
    range.x_begin = width / 10;
    range.x_end = width * 9 / 10;
    range.y_begin = height / 3;
    range.y_end = height * 2 / 3;

    GrayView view(image, range);

    // modify the elements in the view
    invert_gray(view);

    // write the modified image to file
    write_image(image, "modified image path");

    // create a new image from the view
    GrayImage new_image(view.width(), view.height());
    std::copy(view.begin(), view.end(), new_image.begin());

    // write the new image to file
    write_image(new_image, "new image path");
}
```

Our program is going to
* Read an image file in grayscale format
* Make a view from a range in the center of the image
* Invert the the grayscale values of the pixels in the range
* Write the modifed image to file
* Create a new image from the view and write it to file

### the view class

Given our requirements, the GrayView class needs to have the following structure.

```cpp
class GrayView
{
public:    

    GrayView(GrayImage const& image, Range2Du32 const& range); // TODO: implement


    u32 width(); // TODO: implement

    u32 height(); // TODO: implement


    class iterator; // TODO: implement


    iterator begin(); // TODO: implement

    iterator end(); // TODO: implement

    iterator begin() const; // TODO: implement

    iterator end() const; // TODO: implement
};
```

In order to be able to iterate over a 2D range in an image, the view needs pointer to the image's pixel data, the range to iterate over and the width in pixels of the image.  Adding these properties makes implementing the constructor trivial.

```cpp
class GrayView
{
public:

    GrayPixel* image_data_ = nullptr;
    u32 image_width_ = 0;
    Range2Du32 range_ = {};

    GrayView(GrayImage const& image, Range2Du32 const& range)
    {
        image_data_ = image.data;
        image_width_ = image.width;
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

The iterator for the GrayView class needs to be a forward iterator.  That means at minimum it must be able to increment (++), dereference(*) and compare equality (==, !=).  We'll start with the following definitions.

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

In order to track the location within the image we need the x and y positions.  The pointer to the image data is needed as well as the range of the view.  The width of the image is needed for locating the start of each row of pixels.

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
    u32 image_width_ = 0;

    explicit iterator() {}

    explicit iterator(GrayView const& view); // TODO: implement

    iterator& operator ++ (); // TODO: implement

    iterator operator ++ (int); // TODO: implement

    bool operator == (iterator other) const; // TODO: implement

    bool operator != (iterator other) const; // TODO: implement

    reference operator * () const; // TODO: implement
};
```

Constructing the iterator from a view object copies the properties from the view and sets the position to the beginning of the range.

```cpp
explicit iterator(GrayView const& view)
{
    image_data_ = view.image_data_;
    range_ = view.range_;
    image_width_ = view.image_width_;
    
    x_ = range_.x_begin;
    y_ = range_.y_begin;
}
```

To compare equality with another iterator, just check if the x and y positons are the same.

```cpp
bool operator == (iterator other) const { return x_ == other.x_ && y_ == other.y_; }

bool operator != (iterator other) const { return !(*this == other); }
```

To increment, or advance the iterator to the next position, we increment the x position unless we reach the width of the range.  At the end of the range of x values, increment y to the next row and set the x position to the beginning of the range.  We'll place this logic in its own method.

```cpp
void next()
{
    ++x_;
    if (x_ >= range_.x_end)
    {
        ++y_;
        x_ = range_.x_begin;
    }
}
```

The pre-increment operator increments itself and then returns itself;

```cpp
iterator& operator ++ () { next(); return *this; }
```

The post-increment operator increments itself and returns a copy of its state before being incremented.

```cpp
iterator operator ++ (int) { iterator result = *this; ++(*this); return result; }
```

Dereferencing the iterator returns a reference to the pixel at its position.  To do that we need a pointer to the pixel ay its position.  Just like every memory buffer, the pointer to the pixel is at an offset from the beginning of the image buffer.  We calculate the offset using the x and y positions as well as the width of the image.

The number of pixels in each row of the image is the width of the image.  So the offset to the beginning of each row is the y position multiplied by the width.

```cpp
auto image_row_offset = y_ * image_width_;
```

The x position is the number of pixels from the beginning of the row.  That means that the total offset of the pixel is the offset of the row plus the x position.

```cpp
auto offset = image_row_offset + x_;
```

We'll add a method to the iterator class that returns the pixel pointer.

```cpp
GrayPixel* xy_ptr() const
{
    auto image_row_offset = y_ * image_width_;
    auto offset = image_row_offset + x_;

    return image_data_ + (size_t)offset;
}
```

The dereference operator only needs to return the dereferenced pointer of this method.

```cpp
reference operator * () const { return *xy_ptr(); }
```

The last thing the iterator needs is a way to return the "end" of the view.  In the C++ world "begin" is the first element of a collection and "end" is one past the last element.  In this case, the last element is the last pixel in the range.  One past the last pixel is simply the next one.

```cpp
iterator end()
{
    x_ = range_.x_end - 1;
    y = range_.y_end - 1;
    next();

    return *this;
}
```

### Back to the view class

Now that the iterator class is complete, we can turn our attention back to the view.  Including the iterator definition inside of the view class makes things simpler for this post's purposes.

This is what we have so far.

```cpp
class GrayView
{
public:

    GrayPixel* image_data_ = nullptr;
    u32 image_width_ = 0;
    u32 image_height_ = 0;

    Range2Du32 range_ = {};

    GrayView(GrayImage const& image, Range2Du32 const& range)
    {
        image_data_ = image.data;
        image_width_ = image.width;
        image_height_ = image.height;
        range_ = range;
    }


    class iterator
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
        u32 image_width_ = 0;

        explicit iterator() {}

        explicit iterator(GrayView const& view)
        {
            image_data_ = view.image_data_;
            range_ = view.range_;
            image_width_ = view.image_width_;

            x_ = range_.x_begin;
            y_ = range_.y_begin;
        }


        void next()
        {
            ++x_;
            if (x_ >= range_.x_end)
            {
                ++y_;
                x_ = range_.x_begin;
            }
        }


        GrayPixel* xy_ptr() const
        {
            auto image_row_offset = y_ * image_width_;
            auto offset = image_row_offset + x_;
            return image_data_ + (size_t)offset;
        }


        iterator end()
        {
            x_ = range_.x_end - 1;
            y_ = range_.y_end - 1;
            next();

            return *this;
        }


        iterator& operator ++ () { next(); return *this; }

        iterator operator ++ (int) { iterator result = *this; ++(*this); return result; }

        bool operator == (iterator other) const { return x_ == other.x_ && y_ == other.y_; }

        bool operator != (iterator other) const { return !(*this == other); }

        reference operator * () const { return *xy_ptr(); }
    };


    iterator begin(); // TODO: implement

    iterator end(); // TODO: implement

    iterator begin() const; // TODO: implement

    iterator end() const; // TODO: implement
};
```

All that remains are the begin() and end() methods.  These are simple to implement now that the iterator class is complete.  For begin(), we only need to create a new iterator with the view because the position is set to the beginning of the range when constructed.

```cpp
iterator begin() { return iterator(*this); }

iterator begin() const { return iterator(*this); }
```

For the end() methods, we use the iterator's end() method.

```cpp
iterator end() { return iterator(*this).end(); }

iterator end() const { return iterator(*this).end(); }
```

And with that, our GrayView class is complete.

```cpp
class GrayView
{
public:

    GrayPixel* image_data_ = nullptr;
    u32 image_width_ = 0;
    u32 image_height_ = 0;

    Range2Du32 range_ = {};

    GrayView(GrayImage const& image, Range2Du32 const& range)
    {
        image_data_ = image.data;
        image_width_ = image.width;
        image_height_ = image.height;
        range_ = range;
    }


    class iterator
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
        u32 image_width_ = 0;

        explicit iterator() {}

        explicit iterator(GrayView const& view)
        {
            image_data_ = view.image_data_;
            range_ = view.range_;
            image_width_ = view.image_width_;

            x_ = range_.x_begin;
            y_ = range_.y_begin;
        }


        void next()
        {
            ++x_;
            if (x_ >= range_.x_end)
            {
                ++y_;
                x_ = range_.x_begin;
            }
        }


        GrayPixel* xy_ptr() const
        {
            auto image_row_offset = y_ * image_width_;
            auto offset = (size_t)(image_row_offset + x_);
            auto ptr = image_data_ + offset;

            return ptr;
        }


        iterator end()
        {
            x_ = range_.x_end - 1;
            y_ = range_.y_end - 1;
            next();

            return *this;
        }


        iterator& operator ++ () { next(); return *this; }

        iterator operator ++ (int) { iterator result = *this; ++(*this); return result; }

        bool operator == (iterator other) const { return x_ == other.x_ && y_ == other.y_; }

        bool operator != (iterator other) const { return !(*this == other); }

        reference operator * () const { return *xy_ptr(); }
    };


    iterator begin() { return iterator(*this); }

    iterator end() { return iterator(*this).end(); }

    iterator begin() const { return iterator(*this); }

    iterator end() const { return iterator(*this).end(); }
};
```

We are now ready to try out our program.

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
    range.x_begin = width / 10;
    range.x_end = width * 9 / 10;
    range.y_begin = height / 3;
    range.y_end = height * 2 / 3;

    GrayView view(image, range);

    // invert the grayscale colors of the pixels within the view
    invert_gray(view);

    // write the modified image to file
    write_image(image, "new image path");
}
```

Run the program with the following image.

![alt text](https://github.com/adam-lafontaine/CMS/raw/post-9-image-view/blog/img/%5B009%5D/pixel-character.png)

The modifed image is grayscale and its center region's pixel intensities are inverted.

![alt text](https://github.com/adam-lafontaine/CMS/raw/post-9-image-view/blog/img/%5B009%5D/inverted.png)

The new image is a copy of the modified image's center region.

![alt text](https://github.com/adam-lafontaine/CMS/raw/post-9-image-view/blog/img/%5B009%5D/image_from_view.png)




### Epilogue: Reading and writing grayscale images

In a previous post, we covered reading and writing images (https://almostalwaysauto.com/posts/read-write-image).  The examples there were for color images.

The stb_image library allows for reading and writing grayscale images as well.  It also allows reading color images as grayscale images.  The stbi_load function takes a parameter for the number of desired channels in the loaded image.  Choose 4 channels for RGBA images, 3 channel for RGB, and 1 channel for grayscale.  If the image is a color image and 1 channel is selected for the output, the image will be converted to grayscale.

```cpp
void read_image_from_file(const char* img_path_src, GrayImage& image_dst)
{
    int width = 0;
    int height = 0;
    int image_channels = 0;
    int desired_channels = 1; // one channel for grayscale images

    auto data = (GrayPixel*)stbi_load(img_path_src, &width, &height, &image_channels, desired_channels);

    assert(data);
    assert(width);
    assert(height);

    image_dst.data = data;
    image_dst.width = width;
    image_dst.height = height;
}
```

There is similar functionality for writing images to file.

```cpp
void write_image(GrayImage const& image_src, const char* file_path_dst)
{
    assert(image_src.width);
    assert(image_src.height);
    assert(image_src.data);

    int width = (int)image_src.width;
    int height = (int)image_src.height;
    int channels = 1; // one channel for grayscale images
    auto const data = image_src.data;

    int result = 0;

    result = stbi_write_bmp(file_path_dst, width, height, channels, data);

    assert(result);
}
```
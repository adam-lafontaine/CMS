# Basic GUI application
## Using SDL2

This post will walk you through getting a basic application started using SDL2, which is a wrapper around operating system APIs.  The application will open a window and display a generated image based on user input.

### Install SDL2 - Windows

The first thing is to get the SDL2 libraries installed.  On Windows/Visual Studio, it is easiest to use vcpkg.

https://vcpkg.io/en/getting-started.html

Simply clone/download the repository and follow the instructions in the guide.

Run the following commands in **Windows Powershell as administrator**.

Set Visual Studio to automatically include and link installed libraries.

```
.\vcpkg.exe integrate install
```

Install SDL2 (32 bit)

```
.\vcpkg.exe install sdl2
```

Install SDL2 (64 bit)

```
.\vcpkg.exe install sdl2:x64-windows
```

### Install SDL2 - Ubuntu

```
apt-get install libsdl2-dev libsdl2-dbg
```

Include the following in your g++ command line arguments when compiling and linking

```
`sdl2-config --cflags --libs`
```

Note: You may receive an error similar to the following when running your application.

```
dbus[8661]: arguments to dbus_message_new_method_call() were incorrect, assertion "path != NULL" failed in file ../../../dbus/dbus-message.c line 1362.
This is normally a bug in some application using the D-Bus library.

  D-Bus not built with -rdynamic so unable to print a backtrace
```
Internet searches have found that this is a bug with either SDL or Ubuntu.  The workaround is to kill the service in question.

```
sudo killall ibus-daemon
```

If everything is setup properly, the following program should compile and run without errors.

```cpp
#if defined(_WIN32)
#define SDL_MAIN_HANDLED
#endif
#include <SDL2/SDL.h>


int main(int argc, char* args[])
{

}
```

### Application Setup

Before using the libray, SDL needs to be initialized.

```cpp
#include <cstdio>

bool init_sdl()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        printf("SDL_Init failed\n%s\n", SDL_GetError());
        return false;
    }

    return true;
}
```

We'll start a cleanup function to free resources when the program ends.

```cpp
void cleanup()
{
    SDL_Quit();
}
```

Now our main function does nothing but initialize and close the SDL library.

```cpp
int main(int argc, char* args[])
{
    if (!init_sdl())
    {
        return EXIT_FAILURE;
    }

    cleanup();
    return EXIT_SUCCESS;
}
```

### Warning - Memory leak

If we run our program checking for memory leaks, we will find them.

https://almostalwaysauto.com/posts/no-leaks-allowed

Unfortunately, this makes it more difficult for us to check for leaks in our code.

When using a library for the first time, it is a good idea to check for leaks by isolating it in a small program.  It can save time later by preventing you from searching for a leak in your code that doesn't exist.

### Application Setup (continued)

Next, we need a function to create our window.

```cpp
constexpr auto WINDOW_TITLE = "Image Window";
constexpr int WINDOW_WIDTH = 600;
constexpr int WINDOW_HEIGHT = 600;


SDL_Window* create_window()
{
    auto window = SDL_CreateWindow(
        WINDOW_TITLE,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_RESIZABLE);

    if (!window)
    {
        printf("SDL_CreateWindow failed\n%s\n", SDL_GetError());
    }

    return window;
}
```

In main, we'll attempt to create the window if the SDL library was initialized successfully.

```cpp
int main(int argc, char* args[])
{
    if (!init_sdl())
    {
        return EXIT_FAILURE;
    }

    auto window = create_window();
    if (!window)
    {
        cleanup();
        return EXIT_FAILURE;
    }

    cleanup();
    return EXIT_SUCCESS;
}
```

### Set Application Framerate

In general, GUI applications run an infinite loop that reads input, processes the input and other data and renders the frame in the window.  We need to control how frequently the application does this otherwise each frame will render at different speeds and more load than necessary will be placed on the CPU.

60 frames per second is usually a good target.

```cpp
#include <thread>
#include <cstddef>

using u8 = uint8_t;
using u32 = uint32_t;
using r64 = double;

constexpr r64 TARGET_FRAMERATE_HZ = 60.0;
constexpr r64 TARGET_MS_PER_FRAME = 1000.0 / TARGET_FRAMERATE_HZ;
```

To keep things simple we'll start keeping global static variables.  This one will be used to signal when the application should be terminated.

```cpp
static bool g_running = false;
```

In the loop we need a function that pauses each frame for just enough time to maintain our 60 FPS.

```cpp
#include "stopwatch.hpp"


void wait_for_framerate(Stopwatch& sw)
{
    auto frame_ms_elapsed = sw.get_time_milli();
    auto sleep_ms = (u32)(TARGET_MS_PER_FRAME - frame_ms_elapsed);
    if (frame_ms_elapsed < TARGET_MS_PER_FRAME && sleep_ms > 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
        while (frame_ms_elapsed < TARGET_MS_PER_FRAME)
        {
            frame_ms_elapsed = sw.get_time_milli();
        }
    }
    else
    {
        printf("Missed framerate\n");
    }

    sw.start();
}
```

The Stopwatch class is available here:

https://github.com/adam-lafontaine/Cpp_Utilities/blob/master/stopwatch/stopwatch.hpp

Add the loop in main along with some temporary code that terminates the application after running for 5 seconds.

```cpp
int main(int argc, char* args[])
{
    if (!init_sdl())
    {
        return EXIT_FAILURE;
    }

    auto window = create_window();
    if (!window)
    {
        cleanup();
        return EXIT_FAILURE;
    }

    g_running = true;
    Stopwatch sw;
    sw.start();

    // temp
    u32 temp_n_frames = (u32)(5000.0 / TARGET_MS_PER_FRAME); // keep app running for 5 seconds
    u32 temp_frame_count = 0;

    while (g_running)
    {
        // temp
        ++temp_frame_count;
        if (temp_frame_count >= temp_n_frames)
        {
            g_running = false;
        }

        wait_for_framerate(sw);
    }

    cleanup();
    return EXIT_SUCCESS;
}
```

### Keyboard Input

In every frame, SDL checks for an input event and stores the information in a SDL_Event struct.  We will only concern ourselves with keyboard events when the A, B, C or D keys are pressed.

```cpp
void handle_keyboard_event(SDL_Event const& event)
{
    if (event.key.repeat || event.key.state != SDL_PRESSED)
    {
        return;
    }

    auto key_code = event.key.keysym.sym;
    switch (key_code)
    {
    case SDLK_a:
    {
        printf("A\n");
    } break;
    case SDLK_b:
    {
        printf("B\n");
    } break;
    case SDLK_c:
    {
        printf("C\n");
    } break;
    case SDLK_d:
    {
        printf("D\n");
    } break;

    }
}
```

Before handling keyboard events, we first check if the user wishes to quit the application.  The application will terminate if the user clicks the 'X' button in the window tile bar, presses ALT+F4 or ESC.

```cpp
void handle_sdl_event(SDL_Event const& event)
{
    switch (event.type)
    {
    case SDL_QUIT:
    {
        // window X button pressed
        printf("SDL_QUIT\n");
        g_running = false;
    } break;
    case SDL_KEYDOWN:
    case SDL_KEYUP:
    {
        auto key_code = event.key.keysym.sym;
        auto alt = event.key.keysym.mod & KMOD_ALT;
        if (key_code == SDLK_F4 && alt)
        {
            printf("ALT F4\n");
            g_running = false;
        }
        else if (key_code == SDLK_ESCAPE)
        {
            printf("ESC\n");
            g_running = false;
        }
        else
        {
            handle_keyboard_event(event);
        }

    } break;

    }
}
```

In main, replace the temporary frame counting code with code for event handling.

```cpp
int main(int argc, char* args[])
{
    if (!init_sdl())
    {
        return EXIT_FAILURE;
    }

    auto window = create_window();
    if (!window)
    {
        cleanup();
        return EXIT_FAILURE;
    }

    g_running = true;
    Stopwatch sw;
    sw.start();

    while (g_running)
    {
        SDL_Event event;
        if (SDL_PollEvent(&event))
        {
            handle_sdl_event(event);
        }

        wait_for_framerate(sw);
    }

    cleanup();
    return EXIT_SUCCESS;
}
```

### Enable Rendering

For displaying image data in a window, SDL has a texture and a renderer.  The texture holds a reference to the image data and the renderer renders the texture data to the window.  We'll group them together and call it a WindowBuffer.

```cpp
class WindowBuffer
{
public:

    SDL_Renderer* renderer;
    SDL_Texture* texture;
};
```

The renderer and texture need to be initialized at startup and destroyed when the program ends.

```cpp
bool init_window_buffer(WindowBuffer& buffer, SDL_Window* window)
{
    buffer.renderer = SDL_CreateRenderer(window, -1, 0);

    if (!buffer.renderer)
    {
        printf("SDL_CreateRenderer failed\n%s\n", SDL_GetError());
        return false;
    }

    buffer.texture = SDL_CreateTexture(
        buffer.renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        WINDOW_WIDTH,
        WINDOW_HEIGHT);

    if (!buffer.texture)
    {
        printf("SDL_CreateTexture failed\n%s\n", SDL_GetError());
        return false;
    }

    return true;
}


void destroy_window_buffer(WindowBuffer& buffer)
{
    if (buffer.texture)
    {
        SDL_DestroyTexture(buffer.texture);
    }

    if (buffer.renderer)
    {
        SDL_DestroyRenderer(buffer.renderer);
    }
}
```

Add a global WindowBuffer variable.

```cpp
static WindowBuffer g_window_buffer;
```

Update the cleanup function.

```cpp
void cleanup()
{
    destroy_window_buffer(g_window_buffer);
    SDL_Quit();
}
```

Initialize the WindowBuffer in main.

```cpp
int main(int argc, char* args[])
{
    if (!init_sdl())
    {
        return EXIT_FAILURE;
    }

    auto window = create_window();
    if (!window)
    {
        cleanup();
        return EXIT_FAILURE;
    }

    if (!init_window_buffer(g_window_buffer, window))
    {
        cleanup();
        return EXIT_FAILURE;
    }

    g_running = true;
    Stopwatch sw;
    sw.start();

    while (g_running)
    {
        SDL_Event event;
        if (SDL_PollEvent(&event))
        {
            handle_sdl_event(event);
        }

        wait_for_framerate(sw);
    }

    cleanup();
    return EXIT_SUCCESS;
}
```

### Setup the image data

We need to be able to write generated image data to the SDL texture.  To do that, we'll use image and pixel logic that is similar to previous posts.

```cpp
class Pixel
{
public:
    u8 red;
    u8 green;
    u8 blue;
    u8 alpha; // padding
};


class Image
{
public:
    u32 width;
    u32 height;

    Pixel* data;
};


bool create_image(Image& image, u32 width, u32 height)
{
    image.width = width;
    image.height = height;
    image.data = (Pixel*)malloc(sizeof(Pixel) * width * height);

    if (!image.data)
    {
        image.data = nullptr;
        return false;
    }

    return true;
}


void destroy_image(Image& image)
{
    if (image.data != nullptr)
    {
        free(image.data);
        image.data = nullptr;
    }
}


Pixel to_pixel(u8 r, u8 g, u8 b)
{
    Pixel p{};

    p.red = r;
    p.green = g;
    p.blue = b;
    p.alpha = 255;

    return p;
}
```

We need a global image and its width and height will match the application widow's.

```cpp
constexpr u32 IMAGE_WIDTH = WINDOW_WIDTH;
constexpr u32 IMAGE_HEIGHT = WINDOW_HEIGHT;

static Image g_image;
```

Make sure to free the image memory when the application is finished.

```cpp
void cleanup()
{
    destroy_window_buffer(g_window_buffer);
    SDL_Quit();
    destroy_image(g_image);
}
```

Update main to create the image data.

```cpp
int main(int argc, char* args[])
{
    if (!init_sdl())
    {
        return EXIT_FAILURE;
    }

    auto window = create_window();
    if (!window)
    {
        return EXIT_FAILURE;
    }

    if (!init_window_buffer(g_window_buffer, window))
    {
        cleanup();
        return EXIT_FAILURE;
    }

    if (!create_image(g_image, IMAGE_WIDTH, IMAGE_HEIGHT))
    {
        cleanup();
        return EXIT_FAILURE;
    }

    g_running = true;
    Stopwatch sw;
    sw.start();

    while (g_running)
    {
        SDL_Event event;
        if (SDL_PollEvent(&event))
        {
            handle_sdl_event(event);
        }

        wait_for_framerate(sw);
    }

    cleanup();
    return EXIT_SUCCESS;
}
```

### Rendering

Now that everything is setup, we can finally write image data and display it in the window.  Writing an image to the window is done like so.

```cpp
void display_image(Image const& image, WindowBuffer const& buffer)
{
    auto pitch = (int)(image.width * sizeof(Pixel));
    auto error = SDL_UpdateTexture(buffer.texture, 0, (void*)image.data, pitch);
    if (error)
    {
        printf("SDL_UpdateTexture failed\n%s\n", SDL_GetError());
        return;
    }

    SDL_RenderCopy(buffer.renderer, buffer.texture, 0, 0);

    SDL_RenderPresent(buffer.renderer);
}
```

We'll do some fairly simple image generation for our examples.  This function sets the the entire window to a single color.

```cpp
void draw_color(Pixel p)
{
    for (u32 i = 0; i < g_image.width * g_image.height; ++i)
    {
        g_image.data[i] = p;
    }
}
```

This will divide the window into blue, green and red vertical sections.

```cpp
void draw_bgr()
{
    auto blue = to_pixel(0, 0, 255);
    auto green = to_pixel(0, 255, 0);
    auto red = to_pixel(255, 0, 0);

    auto blue_max = g_image.width / 3;
    auto green_max = g_image.width * 2 / 3;

    u32 i = 0;
    for (u32 y = 0; y < g_image.height; ++y)
    {
        for (u32 x = 0; x < g_image.width; ++x)
        {
            if (x < blue_max)
            {
                g_image.data[i] = blue;
            }
            else if (x < green_max)
            {
                g_image.data[i] = green;
            }
            else
            {
                g_image.data[i] = red;
            }

            ++i;
        }
    }
}
```

Decide which drawing function to call based on the keyboard input.

```cpp
void handle_keyboard_event(SDL_Event const& event)
{
    if (event.key.repeat || event.key.state != SDL_PRESSED)
    {
        return;
    }

    auto key_code = event.key.keysym.sym;
    switch (key_code)
    {
    case SDLK_a:
    {
        printf("A\n");

        render_color(to_pixel(255, 0, 0));
    } break;
    case SDLK_b:
    {
        printf("B\n");

        render_color(to_pixel(0, 255, 0));
    } break;
    case SDLK_c:
    {
        printf("C\n");

        render_color(to_pixel(0, 0, 255));
    } break;
    case SDLK_d:
    {
        printf("D\n");

        render_bgr();
    } break;

    }
}
```

Finally, write the selected image data to the window on each frame.

```cpp
int main(int argc, char* args[])
{
    if (!init_sdl())
    {
        return EXIT_FAILURE;
    }

    auto window = create_window();
    if (!window)
    {
        return EXIT_FAILURE;
    }

    if (!init_window_buffer(g_window_buffer, window))
    {
        cleanup();
        return EXIT_FAILURE;
    }

    if (!create_image(g_image, IMAGE_WIDTH, IMAGE_HEIGHT))
    {
        cleanup();
        return EXIT_FAILURE;
    }

    g_running = true;
    Stopwatch sw;
    sw.start();

    while (g_running)
    {
        SDL_Event event;
        if (SDL_PollEvent(&event))
        {
            handle_sdl_event(event);
        }

        display_image(g_image, g_window_buffer);

        wait_for_framerate(sw);
    }

    cleanup();
    return EXIT_SUCCESS;
}
```

Now we can update the window by pressing the A, B, C and D keys.

### Further Study

This post is barely an introduction to SDL2 and my knowledge of it is barely more than this post.  To learn how to implement similar functionality (and more) directly on Windows and Linux, I highly recommend the Handmade Hero series by Casey Muratori.

https://handmadehero.org/

The SDL implementation for this post is based on David Gow's companion series called Handmade Penguin.  

https://davidgow.net/handmadepenguin/
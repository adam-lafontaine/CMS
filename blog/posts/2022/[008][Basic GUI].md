# Basic GUI application
## Using SDL2

This post will walk you through getting a basic application started using SDL2.  SDL2 is a wrapper around operating system APIs.  The application will open a window and display a generated image based on user input.

### Install SDL2 - Windows

The first thing is to get the SDL2 libraries installed.  On Windows/Visual Studio, it is easiest to use vcpkg.

https://vcpkg.io/en/getting-started.html

Simply clone/download the vcpkg repository and follow the instructions in the guide.

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
apt-get install libsdl2-dev
```

Include the following in your g++ command line arguments when compiling and linking

```
`sdl2-config --cflags --libs`
```

Note: You may receive an error similar to the following when running your application on a device with an ARM based processor.

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

On Linux, you can use the following commands to compile and run the program.

```plaintext
# compile
g++ -o sdl2_app -c main.cpp `sdl2-config --cflags --libs`

# run
./sdl2_app
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

For displaying image data in a window, SDL has a texture and a renderer.  The texture holds a reference to the image data and the renderer renders the texture data to the window.  We'll group everything together and call it a `WindowBuffer`.

```cpp
class WindowBuffer
{
public:

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
};


constexpr auto WINDOW_TITLE = "Image Window";
constexpr int WINDOW_WIDTH = 600;
constexpr int WINDOW_HEIGHT = 600;
```

Next, we need a function to create our window, renderer and texture.

```cpp
static bool init_window_buffer(WindowBuffer& buffer, int width, int height)
{
    auto error = SDL_CreateWindowAndRenderer(width, height, 0, &(buffer.window), &(buffer.renderer));
    if(error)
    {
        printf("SDL_CreateWindowAndRenderer/n");
        return false;
    }

    SDL_SetWindowTitle(buffer.window, WINDOW_TITLE);

    buffer.texture = SDL_CreateTexture(
        buffer.renderer,
        SDL_PIXELFORMAT_ABGR8888,
        SDL_TEXTUREACCESS_STREAMING,
        width,
        height);

    if (!buffer.texture)
    {
        printf("SDL_CreateTexture failed\n%s\n", SDL_GetError());
        return false;
    }

    return true;
}
```

The SDL resources will need to be released at the end of the program.

```cpp
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

    if (buffer.window)
    {
        SDL_DestroyWindow(buffer.window);
    }
}
```

Add a global `WindowBuffer` variable and update the cleanup function.

```cpp
static WindowBuffer g_window_buffer;


void cleanup()
{
    destroy_window_buffer(g_window_buffer);
    SDL_Quit();
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

    if (!init_window_buffer(g_window_buffer, WINDOW_WIDTH, WINDOW_HEIGHT))
    {
        cleanup();
        return EXIT_FAILURE;
    }

    cleanup();
    return EXIT_SUCCESS;
}
```

### Set Application Framerate

In general, GUI applications run an infinite loop that reads input, processes the input and other data and renders the frame in the window.  We need to control how frequently the application does this, otherwise each frame will render at different speeds and more load than necessary will be placed on the CPU.

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

We'll signal when the application should stop running with a global flag.

```cpp
static bool g_running = false;
```

In the loop, we need a function that pauses each frame for just enough time to maintain our 60 FPS.

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

    if (!init_window_buffer(g_window_buffer, WINDOW_WIDTH, WINDOW_HEIGHT))
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

In every frame, SDL checks for an input event and stores the information in a `SDL_Event` struct.  We will only concern ourselves with keyboard events when the A, B, C or D keys are pressed.

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

### Setup the image data

We need to be able to write a generated image to the SDL texture.  To do that, we'll use image and pixel logic that is similar to previous posts.

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

    Pixel* data = nullptr;
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


constexpr Pixel to_pixel(u8 r, u8 g, u8 b)
{
    Pixel p{};

    p.red = r;
    p.green = g;
    p.blue = b;
    p.alpha = 255;

    return p;
}
```

Add a global Image object and some constants that we'll be using in our example.  Make sure to free the image memory when the application is finished.

```cpp
static Image g_image;


constexpr auto RED = to_pixel(255, 0, 0);
constexpr auto GREEN = to_pixel(0, 255, 0);
constexpr auto BLUE = to_pixel(0, 0, 255);


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

    if (!init_window_buffer(g_window_buffer, WINDOW_WIDTH, WINDOW_HEIGHT))
    {
        cleanup();
        return EXIT_FAILURE;
    }

    if (!create_image(g_image, WINDOW_WIDTH, WINDOW_HEIGHT))
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
    auto blue_max = g_image.width / 3;
    auto green_max = g_image.width * 2 / 3;

    u32 i = 0;
    for (u32 y = 0; y < g_image.height; ++y)
    {
        for (u32 x = 0; x < g_image.width; ++x)
        {
            if (x < blue_max)
            {
                g_image.data[i] = BLUE;
            }
            else if (x < green_max)
            {
                g_image.data[i] = GREEN;
            }
            else
            {
                g_image.data[i] = RED;
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
        printf("A - red\n");

        draw_color(RED);
    } break;
    case SDLK_b:
    {
        printf("B - green\n");

        draw_color(GREEN);
    } break;
    case SDLK_c:
    {
        printf("C - blue\n");

        draw_color(BLUE);
    } break;
    case SDLK_d:
    {
        printf("D - blue green red\n");

        draw_bgr();
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
# Basic video application
## Using SDL2 with OpenCV

### SDL2

SDL2 code

```cpp


#include <cstdio>

#if defined(_WIN32)
#define SDL_MAIN_HANDLED
#endif
#include <SDL2/SDL.h>


class WindowBuffer
{
public:

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
};


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


static bool init_window_buffer(WindowBuffer& buffer, int width, int height, const char* window_title)
{
    auto error = SDL_CreateWindowAndRenderer(width, height, 0, &(buffer.window), &(buffer.renderer));
    if (error)
    {
        printf("SDL_CreateWindowAndRenderer/n");
        return false;
    }

    SDL_SetWindowTitle(buffer.window, window_title);

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

Create a class to hold the state of the application

```cpp
class AppState
{
public:
    bool is_running = false;
};
```

Handle input from keyboard

```cpp
void handle_keyboard_event(SDL_Event const& event, AppState& state)
{
    if (event.key.repeat || event.key.state != SDL_PRESSED)
    {
        return;
    }

    auto key_code = event.key.keysym.sym;

    switch (key_code)
    {

    default:
        printf("any key\n");
    }
}


void handle_sdl_event(SDL_Event const& event, AppState& state)
{
    switch (event.type)
    {
    case SDL_QUIT:
    {
        printf("SDL_QUIT\n");
        state.is_running = false;
    } break;
    case SDL_KEYDOWN:
    case SDL_KEYUP:
    {
        auto key_code = event.key.keysym.sym;
        auto alt = event.key.keysym.mod & KMOD_ALT;

        switch (key_code)
        {
        case SDLK_F4:
            if (alt)
            {
                printf("ALT F4\n");
                state.is_running = false;
                return;
            }
            break;

        case SDLK_ESCAPE:
            printf("ESC\n");
            state.is_running = false;

            break;

        default:
            handle_keyboard_event(event, state);
        }

    } break;

    }
}
```

Main function along with code to throttle the frame rate.

```cpp
#include "stopwatch.hpp"

#include <cstddef>
#include <thread>


using u8 = uint8_t;
using u32 = uint32_t;
using r32 = float;


void wait_for_framerate(Stopwatch& sw)
{
    constexpr auto target_ms_per_frame = 30.0;

    auto frame_ms_elapsed = sw.get_time_milli();
    auto sleep_ms = (u32)(target_ms_per_frame - frame_ms_elapsed);
    if (frame_ms_elapsed < target_ms_per_frame && sleep_ms > 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
        while (frame_ms_elapsed < target_ms_per_frame)
        {
            frame_ms_elapsed = sw.get_time_milli();
        }
    }

    sw.start();
}


int main()
{
    if (!init_sdl())
    {
        return EXIT_FAILURE;
    }

    auto app_title = "OpenCV SDL2";
    int window_height = 480;
    int window_width = 640;

    WindowBuffer window_buffer;
    AppState state;

    auto const cleanup = [&]() 
    {
        destroy_window_buffer(window_buffer);
        SDL_Quit();
    };

    if (!init_window_buffer(window_buffer, window_width, window_height, app_title))
    {
        cleanup();
        return EXIT_FAILURE;
    }

    state.is_running = true;
    
    Stopwatch sw;
    sw.start();

    while (state.is_running)
    {
        SDL_Event event;
        if (SDL_PollEvent(&event))
        {
            handle_sdl_event(event, state);
        }

        wait_for_framerate(sw);
    }

    cleanup();
    return EXIT_SUCCESS;
}
```

SDL2 pixel format

```cpp
class RGBA
{
public:
    u8 red = 0;
    u8 green = 0;
    u8 blue = 0;
    u8 alpha = 0;
};;


RGBA to_rgba(u8 r, u8 g, u8 b)
{
    return { r, g, b, 255 };
}
```

Image format

```cpp
void destroy_image(ImageRGBA& image)
{
    if (image.data)
    {
        free(image.data);
    }
}


bool create_image(ImageRGBA& image, u32 width, u32 height)
{
    auto data = malloc(sizeof(RGBA) * width * height);
    if (!data)
    {
        return false;
    }

    image.width = width;
    image.height = height;
    image.data = (RGBA*)data;

    return true;
}
```

Write image to window

```cpp
static void render_image(ImageRGBA const& src, WindowBuffer const& dst)
{
    int pitch = src.width * sizeof(RGBA);
    auto error = SDL_UpdateTexture(dst.texture, 0, src.data, pitch);
    if (error)
    {
        printf("SDL_UpdateTexture failed\n");
    }

    error = SDL_RenderCopy(dst.renderer, dst.texture, 0, 0);
    if (error)
    {
        printf("SDL_RenderCopy failed\n");
    }

    SDL_RenderPresent(dst.renderer);
}
```

Add an image to the state.

```cpp
class AppState
{
public:
    bool is_running = false;

    ImageRGBA screen_image;
};
```

Update main to create the screen image and render it every frame.

```cpp
int main()
{
    if (!init_sdl())
    {
        return EXIT_FAILURE;
    }

    auto app_title = "OpenCV SDL2";
    int window_height = 480;
    int window_width = 640;

    WindowBuffer window_buffer;
    AppState state;

    auto const cleanup = [&]() 
    {
        destroy_image(state.screen_image);
        destroy_window_buffer(window_buffer);
        SDL_Quit();
    };

    if (!init_window_buffer(window_buffer, window_width, window_height, app_title))
    {
        cleanup();
        return EXIT_FAILURE;
    }

    if (!create_image(state.screen_image, window_width, window_height))
    {
        cleanup();
        return EXIT_FAILURE;
    }

    state.is_running = true;
    
    Stopwatch sw;
    sw.start();

    while (state.is_running)
    {
        SDL_Event event;
        if (SDL_PollEvent(&event))
        {
            handle_sdl_event(event, state);
        }

        render_image(state.screen_image, window_buffer);

        wait_for_framerate(sw);
    }

    cleanup();
    return EXIT_SUCCESS;
}
```
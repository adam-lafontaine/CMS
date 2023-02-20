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


static bool g_running = false;


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

    }
}


void handle_sdl_event(SDL_Event const& event)
{
    switch (event.type)
    {
    case SDL_QUIT:
    {
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

Main

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
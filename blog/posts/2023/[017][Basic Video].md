# Basic video application
## Using SDL2 with OpenCV


The easy way with OpenCV

```cpp
#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>


void show_webcam()
{
    cv::VideoCapture cap(0);

    cv::Mat frame;

    for(;;)
    {
        cap >> frame;

        if (frame.empty())
        {
            break;
        }

        cv::imshow("OpenCV window", frame);

        if (cv::waitKey(15) == 'q')
        {
            break;
        }
    }
}
```


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




```cpp
#include <algorithm>


void fill_image(ImageRGBA const& image, RGBA color)
{
    auto image_begin = image.data;
    auto image_end = image_begin + image.width * image.height;

    std::fill(image_begin, image_end, color);
}
```

Update the keyboard event handling to fill the screen with a given color when a key is pressed.

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
    case SDLK_r:
        fill_image(state.screen_image, to_rgba(255, 0, 0));
        printf("red\n");
        break;
    case SDLK_g:
        fill_image(state.screen_image, to_rgba(0, 255, 0));
        printf("green\n");
        break;
    case SDLK_b:
        fill_image(state.screen_image, to_rgba(0, 0, 255));
        printf("blue\n");
        break;

    default:
        printf("any key\n");
    }
}
```

The above will write to the state image only when a key is pressed.  To enable video, we need to update the image every frame.

Add a function object to the state.

```cpp
#include <functional>


class AppState
{
public:
    bool is_running = false;

    ImageRGBA screen_image;

    std::function<void()> update_frame = []() {};
};
```

Update the keyboard event handling to change the function when a key pressed.

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
    case SDLK_r:
        state.update_frame = [&]() { fill_image(state.screen_image, to_rgba(255, 0, 0)); };
        printf("red\n");
        break;
    case SDLK_g:
        state.update_frame = [&]() { fill_image(state.screen_image, to_rgba(0, 255, 0)); };
        printf("green\n");
        break;
    case SDLK_b:
        state.update_frame = [&]() { fill_image(state.screen_image, to_rgba(0, 0, 255)); };
        printf("blue\n");
        break;

    default:
        printf("any key\n");
    }
}
```

Call the function in each frame in main.

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

        state.update_frame();

        render_image(state.screen_image, window_buffer);

        wait_for_framerate(sw);
    }

    cleanup();
    return EXIT_SUCCESS;
}
```

### OpenCV



Camera definition.

```cpp
#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>


class Camera
{
public:
    cv::VideoCapture capture;
    cv::Mat frame;

    u32 frame_width = 0;
    u32 frame_height = 0;
};
```

Initialize the camera before using it.

```cpp
bool open_camera(Camera& camera)
{
    auto& cap = camera.capture;

    cap = cv::VideoCapture(0);
    if (!cap.isOpened())
    {
        return false;
    }

    camera.frame_width = (u32)cap.get(cv::CAP_PROP_FRAME_WIDTH);
    camera.frame_height = (u32)cap.get(cv::CAP_PROP_FRAME_HEIGHT);

    return true;
}
```

Add a Camera to the state class.

```cpp
class AppState
{
public:
    bool is_running = false;

    ImageRGBA screen_image;

    std::function<void()> update_frame = []() {};

    Camera camera;
};
```

Update main to open a Camera and set the window width and height based on its frame dimensions.

```cpp
int main()
{
    if (!init_sdl())
    {
        return EXIT_FAILURE;
    }

    auto app_title = "OpenCV SDL2";    

    WindowBuffer window_buffer;
    AppState state;

    if (!open_camera(state.camera))
    {
        return EXIT_FAILURE;
    }

    int window_height = (int)state.camera.frame_height;
    int window_width = (int)state.camera.frame_width;

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

        state.update_frame();

        render_image(state.screen_image, window_buffer);

        wait_for_framerate(sw);
    }

    cleanup();
    return EXIT_SUCCESS;
}
```

OpenCV frames are in BGR format.  We will need to convert it to RGBA in order to write the webcam image to the screen.

```cpp
class BGR
{
public:
    u8 blue;
    u8 green;
    u8 red;
};


RGBA bgr_to_rgba(BGR bgr)
{
    return { bgr.red, bgr.green, bgr.blue, 255 };
}
```

Grab a frame

```cpp
bool grab_frame(Camera& camera)
{
    auto& cap = camera.capture;

    if (!cap.grab())
    {
        return false;
    }

    auto& frame = camera.frame;

    if (!cap.retrieve(frame))
    {
        return false;
    }

    return true;
}
```

If a frame grab was successful, convert the BGR frame to the RGBA image.

```cpp
void grab_and_convert_frame(Camera& camera, ImageRGBA const& image)
{
    if (!grab_frame(camera))
    {
        return;
    }

    auto frame_begin = camera.frame.data;
    auto frame_end = frame_begin + camera.frame_width * camera.frame_height;

    auto image_begin = image.data;

    std::transform(frame_begin, frame_end, image_begin, bgr_to_rgba);
}
```

Update the event handling to display the webcam frame in the window.

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
    case SDLK_r:
        state.update_frame = [&]() { fill_image(state.screen_image, to_rgba(255, 0, 0)); };
        printf("red\n");
        break;
    case SDLK_g:
        state.update_frame = [&]() { fill_image(state.screen_image, to_rgba(0, 0, 255)); };
        printf("green\n");
        break;
    case SDLK_b:
        state.update_frame = [&]() { fill_image(state.screen_image, to_rgba(0, 255, 0)); };
        printf("blue\n");
        break;

    case SDLK_c:
        state.update_frame = [&]() { grab_and_convert_frame(state.camera, state.screen_image); };
        break;

    default:
        printf("any key\n");
    }
}
```
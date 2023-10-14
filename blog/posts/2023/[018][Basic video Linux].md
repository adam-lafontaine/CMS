# Basic video rendering - Linux edition
## Replacing OpenCV with Libuvc

The previous post demonstrated how to capture webcam image frames using OpenCV and then render them in a window using SDL2.

If all we are doing is grabbing video frames, then we may not want to have OpenCV as a dependency in our application.  In this post we'll cover using a library called [Libuvc](https://github.com/libuvc/libuvc) as an alternative.

Note: I have not been able to get this to work on Windows.  This example was run on Linux Ubuntu 20.04.

### SDL2 Recap



```cpp
int main()
{
    if (!init_sdl())
    {
        return EXIT_FAILURE;
    }

    auto app_title = "Libuvc SDL2";
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
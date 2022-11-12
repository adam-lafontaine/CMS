#include <thread>
#include <cstddef>
#include <cstdio>
#include <chrono>
#include <SDL2/SDL.h>
#include <emscripten.h>


using u8 = uint8_t;
using u32 = uint32_t;
using r64 = double;


class Pixel
{
public:
    u8 red;
    u8 green;
    u8 blue;
    u8 alpha; // padding
};


constexpr Pixel to_pixel(u8 r, u8 g, u8 b)
{
    Pixel p{};

    p.red = r;
    p.green = g;
    p.blue = b;
    p.alpha = 255;

    return p;
}


class Image
{
public:
    u32 width;
    u32 height;

    Pixel* data = nullptr;
};


class WindowBuffer
{
public:

    SDL_Renderer* renderer;
    SDL_Texture* texture;
};


class Stopwatch
{
private:
	std::chrono::system_clock::time_point start_;
	std::chrono::system_clock::time_point end_;
	bool is_on_ = false;

	std::chrono::system_clock::time_point now() { return std::chrono::system_clock::now(); }

public:
	Stopwatch()
	{
		start_ = now();
		end_ = start_;
	}

	void start()
	{
		start_ = now();
		is_on_ = true;
	}

	void stop()
	{
		end_ = now();
		is_on_ = false;
	}

	double get_time_milli()
	{

		std::chrono::duration<double, std::milli> delay = is_on_ ? now() - start_ : end_ - start_;

		return delay.count();
	}
};


constexpr auto WINDOW_TITLE = "Image Window";
constexpr int WINDOW_WIDTH = 600;
constexpr int WINDOW_HEIGHT = 600;

constexpr r64 TARGET_FRAMERATE_HZ = 60.0;
constexpr r64 TARGET_MS_PER_FRAME = 1000.0 / TARGET_FRAMERATE_HZ;

constexpr u32 IMAGE_WIDTH = WINDOW_WIDTH;
constexpr u32 IMAGE_HEIGHT = WINDOW_HEIGHT;

constexpr auto RED = to_pixel(255, 0, 0);
constexpr auto GREEN = to_pixel(0, 255, 0);
constexpr auto BLUE = to_pixel(0, 0, 255);


static Image g_image;
static WindowBuffer g_window_buffer;
static bool g_running = false;


// Declarations

bool init_sdl();
SDL_Window* create_window();
bool init_window_buffer(WindowBuffer& buffer, SDL_Window* window);
bool create_image(Image& image, u32 width, u32 height);
void cleanup();
void handle_sdl_event(SDL_Event const& event);
void display_image(Image const& image, WindowBuffer const& buffer);

class Stopwatch;
void wait_for_framerate(Stopwatch& sw);


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


// Implementation

bool init_sdl()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        printf("SDL_Init failed\n%s\n", SDL_GetError());
        return false;
    }

    return true;
}


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


void draw_color(Pixel p)
{
    for (u32 i = 0; i < g_image.width * g_image.height; ++i)
    {
        g_image.data[i] = p;
    }
}


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

        draw_color(RED);
    } break;
    case SDLK_b:
    {
        printf("B\n");

        draw_color(GREEN);
    } break;
    case SDLK_c:
    {
        printf("C\n");

        draw_color(BLUE);
    } break;
    case SDLK_d:
    {
        printf("D\n");

        draw_bgr();
    } break;

    }
}


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


void cleanup()
{
    destroy_window_buffer(g_window_buffer);
    SDL_Quit();
    destroy_image(g_image);
}


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
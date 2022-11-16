# C++ on the web
## Running C/C++ application in a browser


### Installation

https://emscripten.org/docs/getting_started/downloads.html

Install SDL2

```plaintext
sudo apt-get install libsdl2-dev -y
```

Install Python

```plaintext
sudo apt-get install python3
```

Install Git

```plaintext
sudo apt-get install git
```

Clone the Emscripten SDK repo

```plaintext
# Get the emsdk repo
git clone https://github.com/emscripten-core/emsdk.git

```

Run the installation commands

```plaintext
# Enter that directory
cd emsdk

# Fetch the latest version of the emsdk (not needed the first time you clone)
git pull

# Download and install the latest SDK tools.
./emsdk install latest

# Make the "latest" SDK "active" for the current user. (writes .emscripten file)
./emsdk activate latest

# Activate PATH and other environment variables in the current terminal
source ./emsdk_env.sh
```

From the terminal you will be working in

```plaintext
source ../../emsdk/emsdk_env.sh
```

### Makefile setup

https://almostalwaysauto.com/posts/makefiles

Create a file called Makefile

```Makefile
GPP   := g++
build := ./build
code  := ./src

exe_name := hello_earth

program_exe := $(build)/$(exe_name)


main_c       := $(code)/main.cpp
main_o       := $(build)/main.o
object_files := $(main_o)


$(main_o): $(main_c)
	@echo "\n main"
	$(GPP) -o $@ -c $<

$(program_exe): $(object_files)
	@echo "\n $(exe_name)"
	$(GPP) -o $@ $+


build: $(program_exe)

run: build
	$(program_exe)
	@echo "\n"

setup:
	mkdir -p $(build)
	@echo "\n"

clean:
	rm -rfv $(build)/*
	@echo "\n"
```


Create a new folder called 'src' and add a file called main.cpp.

```cpp
#include <cstdio>

int main()
{
    printf("Hello, Earth\n");
}
```

Create the build directory by running make setup

```plaintext
$ make setup
mkdir -p ./build
```

Build the program

```plaintext
$ make build

 main
g++ -o build/main.o -c src/main.cpp

 hello_earth
g++ -o build/hello_earth build/main.o
```

Run the program

```plaintext
$ make run
./build/hello_earth
Hello, Earth
```

### Emscripten

Clean the project

```plaintext
$ make clean
rm -rfv ./build/*
removed './build/hello_earth'
removed './build/main.o'
```

Modify

```makefile
# Emscripten compiler
EPP := em++

build := ./build
code  := ./src

# 'web' directory for the generated html file
web := ./web

exe_name := sdl2_wasm

# the html output location
program_exe := $(web)/$(exe_name).html


main_c       := $(code)/main.cpp
main_o       := $(build)/main.o
object_files := $(main_o)


EPP_FLAGS := -s USE_SDL=2


$(main_o): $(main_c)
	@echo "\n main"
	$(EPP) $(EPP_FLAGS) -o $@ -c $<

$(program_exe): $(object_files)
	@echo "\n $(exe_name)"
	$(EPP) $(EPP_FLAGS) -o $@ $+


build: $(program_exe)

setup:
	mkdir -p $(build)
	mkdir -p $(web)
	@echo "\n"

clean:
	rm -rfv $(build)/*
	rm -rfv $(web)/*
	@echo "\n"
```



Setup again

```plaintext
$ make setup
mkdir -p ./build
mkdir -p ./web
```

Build

```plaintext
$ make build

 main
em++ -s USE_SDL=2 -o build/main.o -c src/main.cpp

 sdl2_wasm
em++ -s USE_SDL=2 -o web/sdl2_wasm.html build/main.o
```

Serve the html file

```plaintext
$ cd {your path}/web/
$ python3 -m http.server 8080
Serving HTTP on 0.0.0.0 port 8080 (http://0.0.0.0:8080/) ...
```

In the browser navigate to localhost:8080/hello_earth.html

![alt text](https://github.com/adam-lafontaine/CMS/raw/p15-cpp-web/blog/img/%5B015%5D/em_html.png)



### Using SDL2

```cpp
#include <cstddef>


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
```

CanvasBuffer

```cpp
#include <SDL2/SDL.h>


class CanvasBuffer
{
public:

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
};
```

Initialize the SDL2 library

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

Free SDL resources

```cpp
void destroy_canvas_buffer(CanvasBuffer& buffer)
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

Write an image to the canvas

```cpp
void display_image(Image const& image, CanvasBuffer const& buffer)
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

Program objects and cleanup

```cpp
static Image g_image;
static CanvasBuffer g_canvas;
static bool g_running = false;


void cleanup()
{
    destroy_canvas_buffer(g_canvas);
    SDL_Quit();
    destroy_image(g_image);
}
```


Constants

```cpp
constexpr int WINDOW_WIDTH = 600;
constexpr int WINDOW_HEIGHT = 600;

constexpr auto RED = to_pixel(255, 0, 0);
constexpr auto GREEN = to_pixel(0, 255, 0);
constexpr auto BLUE = to_pixel(0, 0, 255);
```

Fill the canvas with a single color

```cpp
void draw_color(Pixel p)
{
    for (u32 i = 0; i < g_image.width * g_image.height; ++i)
    {
        g_image.data[i] = p;
    }
}
```

Draw 3 vertical sections of blue green and red.

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


Keyboard events

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

Handle the event from the main loop.  First check for shutdown commands before handling other keyboard events.

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

Main loop

```cpp
#include <emscripten.h>


static void main_loop()
{
    SDL_Event event;
    bool has_event = SDL_PollEvent(&event);
    if (has_event)
    {
        handle_sdl_event(event);
    }

    display_image(g_image, g_canvas);

    if (!g_running)
    {
        emscripten_cancel_main_loop();
    }
}
```

Main function

```cpp
int main(int argc, char* args[])
{
    if (!init_sdl())
    {
        return EXIT_FAILURE;
    }

    if (!init_canvas_buffer(g_canvas, WINDOW_WIDTH, WINDOW_HEIGHT))
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

    emscripten_set_main_loop(main_loop, 0, 1);

    cleanup();
    return EXIT_SUCCESS;
}
```

Generated html

![alt text](https://github.com/adam-lafontaine/CMS/raw/p15-cpp-web/blog/img/%5B015%5D/sdl2_html.png)

Working program

```cpp
#include <cstddef>
#include <cstdio>
#include <cassert>
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


class CanvasBuffer
{
public:

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
};


bool init_sdl()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        printf("SDL_Init failed\n%s\n", SDL_GetError());
        return false;
    }

    return true;
}


static bool init_canvas_buffer(CanvasBuffer& buffer, int width, int height)
{
    auto error = SDL_CreateWindowAndRenderer(width, height, 0, &(buffer.window), &(buffer.renderer));
    if(error)
    {
        printf("SDL_CreateWindowAndRenderer/n");
        return false;
    }

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


void destroy_canvas_buffer(CanvasBuffer& buffer)
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


void display_image(Image const& image, CanvasBuffer const& buffer)
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


static Image g_image;
static CanvasBuffer g_canvas;
static bool g_running = false;


void cleanup()
{
    destroy_canvas_buffer(g_canvas);
    SDL_Quit();
    destroy_image(g_image);
}


constexpr int WINDOW_WIDTH = 600;
constexpr int WINDOW_HEIGHT = 600;

constexpr auto RED = to_pixel(255, 0, 0);
constexpr auto GREEN = to_pixel(0, 255, 0);
constexpr auto BLUE = to_pixel(0, 0, 255);


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


static void main_loop()
{
    SDL_Event event;
    bool has_event = SDL_PollEvent(&event);
    if (has_event)
    {
        handle_sdl_event(event);
    }

    display_image(g_image, g_canvas);

    if (!g_running)
    {
        emscripten_cancel_main_loop();
    }
}



int main(int argc, char* args[])
{
    if (!init_sdl())
    {
        return EXIT_FAILURE;
    }

    if (!init_canvas_buffer(g_canvas, WINDOW_WIDTH, WINDOW_HEIGHT))
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

    emscripten_set_main_loop(main_loop, 0, 1);

    cleanup();
    return EXIT_SUCCESS;
}
```


Working makefile

```makefile

# source ../../emsdk/emsdk_env.sh
# python3 -m http.server 8080

# Emscripten compiler
EPP := em++

build := ./build
code  := ./src

# 'web' directory for the generated html file
web := ./web

exe_name := sdl2_wasm

# the html output location
program_exe := $(web)/$(exe_name).html


main_c       := $(code)/main.cpp
main_o       := $(build)/main.o
object_files := $(main_o)


EPP_FLAGS := -s USE_SDL=2


$(main_o): $(main_c)
	@echo "\n main"
	$(EPP) $(EPP_FLAGS) -o $@ -c $<

$(program_exe): $(object_files)
	@echo "\n $(exe_name)"
	$(EPP) $(EPP_FLAGS) -o $@ $+


build: $(program_exe)

setup:
	mkdir -p $(build)
	mkdir -p $(web)
	@echo "\n"

clean:
	rm -rfv $(build)/*
	rm -rfv $(web)/*
	@echo "\n"
```
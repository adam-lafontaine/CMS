# C++ on the web
## Porting a C/C++ application to the browser

### Emscripten

C and C++ used to be only for computers and embedded devices.  It is now possible to run "native" applications in a web page.  Emscripten compiles code to Web Assembly (wasm) which will run in modern browsers.  It supports SDL2 so any game or desktop application that uses it can be made to run on the web with very little effort.

In a [previous post](https://almostalwaysauto.com/posts/basic-gui) we covered how to get a basic GUI app up and running with SDL2.  Here we'll walk through getting that app working in the browser.

### Basic setup

The code in this post is compiled and run on Ubuntu.  We'll start with a simple program that runs in the console and change it as we go.

Create a file called Makefile containing the following.  It uses the normal C++ compiler and expects a file called main.cpp in the /src directory.

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

The basics of Makefiles are covered here: https://almostalwaysauto.com/posts/makefiles.

Create a new folder called 'src' and add the file main.cpp.

```cpp
#include <cstdio>


int main()
{
    printf("Hello, Earth\n");
}
```

Create the build directory by running `make setup`

```plaintext
$ make setup
mkdir -p ./build
```

Build the program with `make build`

```plaintext
$ make build

 main
g++ -o build/main.o -c src/main.cpp

 hello_earth
g++ -o build/hello_earth build/main.o
```

Run the program with `make run` to get the expected output.

```plaintext
$ make run
./build/hello_earth
Hello, Earth
```

Clean the project.

```plaintext
$ make clean
rm -rfv ./build/*
removed './build/hello_earth'
removed './build/main.o'
```

### Installing Emscripten

The Empscripten library is on Github so in order to get it, you'll need to have Git installed.

```plaintext
sudo apt install git
```

The installation commands below are taken directly from the [Emscripten website](https://emscripten.org/docs/getting_started/downloads.html).  Chose a location where you want to download emscripten and run the following commands.

```plaintext
# Get the emsdk repo
git clone https://github.com/emscripten-core/emsdk.git

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

The final `source` command will allow you to use emscripten in the current terminal.  The command will need to be run in each new terminal pointing to the correct location of the file.

Emsripten generates an html file.  The easiest way to serve the file is with a Python http server.  Install Python if it isn't installed already.

```plaintext
sudo apt install python3
```

Modify the makefile to use the Empscripten compiler.  There will also be a 'web' folder to store the generated html and supporting files.

```makefile
# Emscripten compiler
EPP := em++

build := ./build
code  := ./src

# 'web' directory for the generated html file
web := ./web

exe_name := hello_earth

# the html output location
program_exe := $(web)/$(exe_name).html


main_c       := $(code)/main.cpp
main_o       := $(build)/main.o
object_files := $(main_o)

# compilation flags TODO
EPP_FLAGS :=


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

The `run` rule no longer applies because the program automatically runs in the browser when the page is loaded.

Run `make setup` to create the new web directory as well.

```plaintext
$ make setup
mkdir -p ./build
mkdir -p ./web
```

Now `make build` will compile the program as before and also generate .html, .js, and .wasm files.

```plaintext
$ make build

 main
em++  -o build/main.o -c src/main.cpp
shared:INFO: (Emscripten: Running sanity checks)

 sdl2_wasm
em++  -o web/hello_earth.html build/main.o
```

In another terminal, navigate to the /web directory and run the http server.

```plaintext
$ cd {your path}/web/
$ python3 -m http.server 8080
Serving HTTP on 0.0.0.0 port 8080 (http://0.0.0.0:8080/) ...
```

In a browser, navigate to localhost:8080/hello_earth.html

![alt text](https://github.com/adam-lafontaine/CMS/raw/p16-cpp-web/blog/img/%5B016%5D/em_html.png)

This is the default web page generated by Emscripten.  The console output from our program is written to a text area in the bottom section.  The top section is a canvas element where any generated graphics are displayed.

### Using SDL2

Emscripten works with many popular C++ libraries including SDL2.  Rather than image data being written to a window buffer, it will be written to an html canvas element.

Install SDL2 if it isn't already.

```plaintext
sudo apt install libsdl2-dev -y
```

Update main.cpp to test the SDL2 installation.

```cpp
#include <cstdio>
#include <SDL2/SDL.h>

int main()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        printf("SDL_Init failed\n%s\n", SDL_GetError());
        return EXIT_FAILURE;
    }
    
    printf("SDL2 OK!\n");

    SDL_Quit();

    return EXIT_SUCCESS;
}
```

Change name of the 'executable' and update the makefile compilation flags to use the SDL2 library

```makefile
exe_name := sdl2_wasm

# compilation flags
EPP_FLAGS := -s USE_SDL=2
```

Run `make build` and launch the new page in the browser.  

![alt text](https://github.com/adam-lafontaine/CMS/raw/p16-cpp-web/blog/img/%5B016%5D/sdl2_test.png)


### Porting the application

Now that we have SDL2 installed and file generation working, we can begin porting our desktop application to the web.  The code we'll start with is from this post.

https://almostalwaysauto.com/posts/basic-gui

Define a 4 byte pixel and an image that the application draws to.

```cpp
#include <cstddef>


using u8 = uint8_t;
using u32 = uint32_t;


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

The image that the application writes to will be copied to the canvas element on the html page.  The SDL2 library takes care of this for us.  We'll store the SDL2 references in a single structure and call it a CanvasBuffer.

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

The SDL2 library needs to be initialized just like before.

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

The same API functions are also used to initialize the SDL2 resources.

```cpp
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
```

When finished, we'll free the SDL resources.

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

Rendering our image in the canvas is identical to rendering it in a window.

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

To keep things simple, we'll have global properties.

```cpp
static Image g_image;
static CanvasBuffer g_canvas;
static bool g_running = false;
```

Clean up all resources at the end of the program.

```cpp
void cleanup()
{
    destroy_canvas_buffer(g_canvas);
    SDL_Quit();
    destroy_image(g_image);
}
```
Define some constants.

```cpp
constexpr int WINDOW_WIDTH = 600;
constexpr int WINDOW_HEIGHT = 600;

constexpr auto RED = to_pixel(255, 0, 0);
constexpr auto GREEN = to_pixel(0, 255, 0);
constexpr auto BLUE = to_pixel(0, 0, 255);
```

In order to fill the canvas with a single color, we iterate over the entire image and set each pixel to that color.

```cpp
void draw_color(Pixel p)
{
    for (u32 i = 0; i < g_image.width * g_image.height; ++i)
    {
        g_image.data[i] = p;
    }
}
```

We'll also have the option to draw 3 vertical sections of blue green and red.

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

Keyboard events are handled exaclty as before.  The application will respond to `A`, `B`, `C`, and `D` keystrokes.

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

Before checking for our keyboard events, first check for shutdown commands.  These aren't as applicable in the context of running on a web page but they function all the same.

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

Emscripten handles the framerate of an application so there is no need run an infinite loop in the main function and track the execution time of each frame.  We do need to define a function that executes in its loop and a means to exit if the application finishes.

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

The API function `emscripten_cancel_main_loop()` sends the signal to stop the Emscripten loop.

Use `emscripten_set_main_loop()` to initiate the application loop from main.

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

Run `make build` once again and refresh the browser to see the application run as it did in the previous post.

![alt text](https://github.com/adam-lafontaine/CMS/raw/p16-cpp-web/blog/img/%5B016%5D/em_sdl2_html.png)

### It's too easy

Having support for SDL2 makes porting an existing application to the web very easy.  All of the application logic stays the same and only a couple of minor changes to the main function are required to get it running in a web browser.

Much more is possible with Emsripten.  Check out https://emscripten.org/docs/getting_started/Tutorial.html to learn more.
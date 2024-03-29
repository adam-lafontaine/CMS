# Basic video rendering - Linux edition
## Replacing OpenCV with libuvc

The [previous post](https://almostalwaysauto.com/posts/basic-video) demonstrated how to capture webcam image frames using OpenCV and then render them in a window with SDL2.

We may not want to have OpenCV as a dependency if all we are doing is grabbing video frames.  In this post we'll use a library called [libuvc](https://github.com/libuvc/libuvc) as an alternative when programming on Linux.  The example presented here was done using g++-11 on Ubuntu 20.04.


### libuvc installation/setup

To start, libuvc requires libusb and libjpeg installed.

```plaintext
sudo apt install -y libusb-1.0-0-dev libjpeg-dev
```

The libuvc readme provides pretty simple installation instructions using CMake.  If you do not have CMake installed or simply do not want to use it, I have created a single header file with all of the library code in it.

[libuvc single header](https://github.com/adam-lafontaine/Cpp_Utilities/blob/master/libuvc/libuvc.h)

Be advised that the single header file does not work on Windows.  Also, be sure to define `LIBUVC_IMPLEMENTATION` before including it.

```cpp
#define LIBUVC_IMPLEMENTATION
#include "libuvc.h"
```

### List devices

First we'll list all of the connected webcams to the console to make sure that everything is installed properly and we're communicating with the cameras.

Create a `DeviceList` struct to keep our references to the libuvc library.

```cpp
#define LIBUVC_IMPLEMENTATION
#include "libuvc.h"

#include <cstdio>


class DeviceList
{
public:
    uvc_context_t* context = nullptr;
    uvc_device_t** device_list = nullptr;

    int n_devices = 0;
};
```

In `main()`, initialize the library, get a reference to a list of the connected devices and then print some information about them to the console.

```cpp
int main()
{
    DeviceList list{};

    auto res = uvc_init(&list.context, NULL);
    if (res != UVC_SUCCESS)
    {
        uvc_perror(res, "uvc_init()");
        uvc_exit(list.context);
        return 1;
    }

    res = uvc_get_device_list(list.context, &list.device_list);
    if (res != UVC_SUCCESS)
    {
        uvc_perror(res, "uvc_get_device_list()");
        uvc_exit(list.context);
        return 1;
    }

    if (!list.device_list || !list.device_list[0])
    {
        printf("No devices found\n");
        uvc_exit(list.context);
        return 1;
    }

    list.n_devices = 0;    

    printf("Devices:\n");
    for (int i = 0; list.device_list[i]; i++) 
    {
        list.n_devices++;

        uvc_device_descriptor_t* desc;

        res = uvc_get_device_descriptor(list.device_list[i], &desc);
        if (res != UVC_SUCCESS)
        {
            uvc_perror(res, "uvc_get_device_descriptor()");
            continue;
        }

        auto vendor_id = (int)desc->idVendor;
        auto product_id = (int)desc->idProduct;        

        printf("  Vendor ID: 0x%04x\n", vendor_id);
        printf(" Product ID: 0x%04x\n", product_id);
        printf("\n");

        uvc_free_device_descriptor(desc);
    }

    uvc_free_device_list(list.device_list, 0);
    
    uvc_exit(list.context);
    return 0;
}
```

When compiling libuvc, we need to include the follow linker flags.

```plaintext
`pkg-config --libs --cflags libusb-1.0` -ljpeg -pthread
```

Here is a sample command line instruction to compile the program.

```plaintext
g++-11 main.cpp -o camera_app `pkg-config --libs --cflags libusb-1.0` -ljpeg -pthread
```

The output should display the vendor ids and and product ids in hexadecimal form.  These will be required in the next step.  My machine currently has 3 webcams connected.

```plaintext
Devices:
  Vendor ID: 0x5986
 Product ID: 0x211b

  Vendor ID: 0x046d
 Product ID: 0x082b

  Vendor ID: 0x0c45
 Product ID: 0x64ab
```

### Set device permissions

Permission needs to be given explicitly for each webcam we want to use.  We do this by creating a file in the `/etc/udev/rules.d/` directory.

```plaintext
/etc/udev/rules.d/99-uvc.rules
```

Create the file called `99-uvc.rules` and for each webcam add the following line.

```plaintext
SUBSYSTEMS=="usb", ENV{DEVTYPE}=="usb_device", ATTRS{idVendor}=="XXXX", ATTRS{idProduct}=="YYYY", MODE="0666"
```
Replace XXXX and YYYY with the 4 hexadecimal characters corresponding to the vendor and product ID of your webcams.  **Restart the computer** for the changes to take effect.

We can rewrite the code to just print the information we need to the console.

```cpp
#include <vector>


bool find_devices(DeviceList& list)
{
    auto res = uvc_init(&list.context, NULL);
    if (res != UVC_SUCCESS)
    {
        uvc_perror(res, "uvc_init()");
        uvc_exit(list.context);
        return false;
    }

    res = uvc_get_device_list(list.context, &list.device_list);
    if (res != UVC_SUCCESS)
    {
        uvc_perror(res, "uvc_get_device_list()");
        uvc_exit(list.context);
        return false;
    }

    if (!list.device_list || !list.device_list[0])
    {
        printf("No devices found\n");
        uvc_exit(list.context);
        return false;
    }

    list.n_devices = 0;
    for (int i = 0; list.device_list[i]; i++)
    {
        list.n_devices++;
    }

    return true;
}


void close_device_list(DeviceList& list)
{
    uvc_free_device_list(list.device_list, 0);
    uvc_exit(list.context);

    list.context = nullptr;
    list.device_list = nullptr;
    list.n_devices = 0;
}


void print_device_list(DeviceList const& list)
{
    class VP
    {
    public:
        int v = 0;
        int p = 0;
    };

    std::vector<VP> vp;
    vp.reserve(list.n_devices);    

    printf("Devices:\n");
    for (int i = 0; i < list.n_devices; i++)
    {
        uvc_device_descriptor_t* desc;

        auto res = uvc_get_device_descriptor(list.device_list[i], &desc);
        if (res != UVC_SUCCESS)
        {
            uvc_perror(res, "uvc_get_device_descriptor()");
            continue;
        }

        auto vendor_id = (int)desc->idVendor;
        auto product_id = (int)desc->idProduct;
        vp.push_back({ vendor_id, product_id });

        printf("  Vendor ID: 0x%04x\n", vendor_id);
        printf(" Product ID: 0x%04x\n", product_id);
        printf("\n");

        uvc_free_device_descriptor(desc);
    }

    printf("Permissions file: /etc/udev/rules.d/99-uvc.rules\n\n");

    auto const fmt = "SUBSYSTEMS==\"usb\", ENV{DEVTYPE}==\"usb_device\", ATTRS{idVendor}==\"%04x\", ATTRS{idProduct}==\"%04x\", MODE=\"0666\"\n";
    for (auto const& item : vp)
    {
        printf(fmt, item.v, item.p);
    }
}


int main()
{
    DeviceList list{};

    if (!find_devices(list))
    {
        return 1;
    }

    print_device_list(list);

    close_device_list(list);

    return 0;
}
```

Here is my updated output for the 3 cameras.

```plaintext
Devices:
  Vendor ID: 0x5986
 Product ID: 0x211b

  Vendor ID: 0x046d
 Product ID: 0x082b

  Vendor ID: 0x0c45
 Product ID: 0x64ab

Permissions file: /etc/udev/rules.d/99-uvc.rules

SUBSYSTEMS=="usb", ENV{DEVTYPE}=="usb_device", ATTRS{idVendor}=="5986", ATTRS{idProduct}=="211b", MODE="0666"
SUBSYSTEMS=="usb", ENV{DEVTYPE}=="usb_device", ATTRS{idVendor}=="046d", ATTRS{idProduct}=="082b", MODE="0666"
SUBSYSTEMS=="usb", ENV{DEVTYPE}=="usb_device", ATTRS{idVendor}=="0c45", ATTRS{idProduct}=="64ab", MODE="0666""
```

Create the rules file to ensure the app can communicate with the cameras.  Remember to restart the computer.


### SDL2 Recap

The supporting code for rendering with SDL2 is identical to that of the [previous post](https://almostalwaysauto.com/posts/basic-video). We'll start at the point where we can set a callback function that updates the image in the window.

```cpp
#include <functional>


class AppState
{
public:
    bool is_running = false;

    ImageRGBA screen_image;

    std::function<void()> update_frame = []() {};
};


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

Our main function is setup to display a 640 x 480 window to start.

```cpp
int main()
{
    if (!init_sdl())
    {
        return 1;
    }

    auto app_title = "libuvc SDL2";
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
        return 1;
    }

    if (!create_image(state.screen_image, window_width, window_height))
    {
        cleanup();
        return 1;
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
    return 0;
}
```

SDL2 requires the following linker flags for compiling.

```plaintext
`sdl2-config --cflags --libs`
```

Here is an updated sample command.

```plaintext
g++-11 main.cpp -o camera_app `pkg-config --libs --cflags libusb-1.0` -ljpeg -pthread `sdl2-config --cflags --libs
```

### Pixel format

Unlike OpenCV, libuvc does not provide a fixed pixel format that it stores frame data in.  Instead it provides the encoding information from the camera and access to the raw data.  It is our responsibility to convert the image data to the format we need.

We won't be covering the various image formats in this post.  To keep things simple, we'll use a libuvc helper function called `uvc_any2rgb()`.  The function checks the frame for one of the common webcam formats.  If it finds a match, it does the appropriate conversion to RGB.

If the frame is not one of the supported image formats, then `uvc_any2rgb()` will fail.  Dealing with this case is outside of the scope of this post but it'll work for most webcams on the market.

Define a RGB image to write the frame data to.

```cpp
#include <cstddef>

using u8 = uint8_t;
using u32 = uint32_t;


class RGB
{
public:
    u8 red = 0;
    u8 green = 0;
    u8 blue = 0;
};


class ImageRGB
{
public:
    u32 width = 0;
    u32 height = 0;

    RGB* data = nullptr;

    uvc_frame_t* uvc_frame;
};
```

Note that the image struct contains a pointer to a `uvc_frame_t`.  This is because `uvc_any2rgb` writes the converted image data to another frame instead of simply a supplied buffer.

To get around this, we point our RGB data to that of the frame.

```cpp
bool create_image(ImageRGB& image, u32 width, u32 height)
{
    image.uvc_frame = uvc_allocate_frame(sizeof(RGB) * width * height);
    if (!image.uvc_frame)
    {
        return false;
    }

    image.width = width;
    image.height = height;
    image.data = (RGB*)image.uvc_frame->data;

    return true;
}


void destroy_image(ImageRGB& image)
{
    if (image.uvc_frame)
    {
        uvc_free_frame(image.uvc_frame);
    }
}
```

The application window image is in RGBA format.  Conversion is very straighforward.

```cpp
RGBA rgb_to_rgba(RGB rgb)
{
    return to_rgba(rgb.red, rgb.green, rgb.blue);
}
```

### Camera setup

We need to define a `Camera` struct that has the frame data in RGB format and the properties required by libuvc to communicate with the physical device.

```cpp
class Camera
{
public:
    uvc_device_t* p_device = nullptr;
    uvc_device_handle_t* h_device = nullptr;
    uvc_stream_handle_t* h_stream = nullptr;

    uvc_stream_ctrl_t ctrl;

    int frame_width = -1;
    int frame_height = -1;
    int fps = -1;

    ImageRGB rgb_frame;

    bool has_device = false;
    bool has_frame = false;
    bool is_streaming = false;
};
```

Our `DeviceList` from earlier will provide access to a `Camera`.  We first do all of the necessary libuvc setup.  

```cpp
#include <cassert>


bool get_default_device(DeviceList const& list, Camera& camera)
{
    assert(list.n_devices && list.device_list[0]);

    // select first camera
    auto device = list.device_list[0];     

    auto res = uvc_open(device, &camera.h_device);
    if (res != UVC_SUCCESS)
    {
        // most likely a permissions issue
        uvc_perror(res, "uvc_open()");
        print_device_list(list);
        return false;
    }

    camera.p_device = device;
    camera.has_device = true;

    return true;
}
```

Query libuvc for the camera information.  If successful, allocate memory for the image data.

```cpp
bool get_frame_properties(Camera& camera)
{
    const uvc_format_desc_t* format_desc = uvc_get_format_descs(camera.h_device);
    const uvc_frame_desc_t* frame_desc = format_desc->frame_descs;

    uvc_frame_format format;
    int width = 640;
    int height = 480;
    int fps = 30;

    switch (format_desc->bDescriptorSubtype) 
    {
    case UVC_VS_FORMAT_MJPEG:
        format = UVC_FRAME_FORMAT_MJPEG;        
        break;
    case UVC_VS_FORMAT_FRAME_BASED:
        format = UVC_FRAME_FORMAT_H264;
        break;
    default:
        format = UVC_FRAME_FORMAT_YUYV;
        break;
    }

    if (frame_desc) 
    {
        width = frame_desc->wWidth;
        height = frame_desc->wHeight;
        fps = 10000000 / frame_desc->dwDefaultFrameInterval;
    }    

    auto res = uvc_get_stream_ctrl_format_size(
        camera.h_device, &camera.ctrl, /* result stored in ctrl */
        format,
        width, height, fps
    );

    if (res != UVC_SUCCESS)
    {
        uvc_perror(res, "uvc_get_stream_ctrl_format_size()");
        return false;
    }

    if (!create_image(camera.rgb_frame, (u32)width, (u32)height))
    {
        printf("Error create_image()\n");
        return false;
    }

    camera.frame_width = width;
    camera.frame_height = height;
    camera.fps = fps;
    camera.has_frame = true;

    return true;
}
```

Start the camera for streaming video.

```cpp
bool start_camera(Camera& camera)
{
    auto res = uvc_stream_open_ctrl(camera.h_device, &camera.h_stream, &camera.ctrl);
    if (res != UVC_SUCCESS)
    {
        uvc_perror(res, "uvc_stream_open_ctrl()");
        return false;
    }

    // specify a callback for processing frames here if wanted
    uvc_frame_callback_t* cb = 0;
    auto cb_user_data = (void*)12345;

    res = uvc_stream_start(camera.h_stream, cb, cb_user_data, 0);
    if (res != UVC_SUCCESS)
    {
        uvc_perror(res, "uvc_stream_start()");
        uvc_stream_close(camera.h_stream);
        return false;
    }

    camera.is_streaming = true;

    return true;
}
```

Cleanup the camera resources when finished.

```cpp
void close_camera(Camera& camera)
{
    if (camera.is_streaming)
    {
        uvc_stop_streaming(camera.h_device);        
    }

    if (camera.has_frame)
    {
        destroy_image(camera.rgb_frame);
    }

    if (camera.has_device)
    {
        uvc_close(camera.h_device);
        uvc_unref_device(camera.p_device);        
    }

    camera.h_stream = nullptr;
    camera.h_device = nullptr;
    camera.p_device = nullptr;    

    camera.frame_width = -1;
    camera.frame_height = -1;
    camera.fps = -1;

    camera.has_device = false;
    camera.has_frame = false;
    camera.is_streaming = false;
}
```

Wrap the device and camera setup logic into one function.

```cpp
bool open_camera(DeviceList& list, Camera& camera)
{   
    if (!get_default_device(list, camera))
    {
        close_camera(camera);
        return false;
    }

    if (!get_frame_properties(camera))
    {
        close_camera(camera);
        return false;
    }

    if (!start_camera(camera))
    {
        close_camera(camera);
        return false;
    }

    return true;
}
```

### Update the app

Add a `DeviceList` and `Camera` to the state class.

```cpp
class AppState
{
public:
    bool is_running = false;

    ImageRGBA screen_image;

    std::function<void()> update_frame = []() {};

    DeviceList device_list;
    Camera camera;
};
```

Update main to find the connected devices and then open a `Camera`.  Set the window width and height based on the camera's frame dimensions.

```cpp
int main()
{
    if (!init_sdl())
    {
        return 1;
    }

    auto app_title = "libuvc SDL2";

    WindowBuffer window_buffer;
    AppState state;

    if (!find_devices(state.device_list))
    {
        return 1;
    }

    if (!open_camera(state.device_list, state.camera))
    {
        return 1;
    }

    int window_height = (int)state.camera.frame_height;
    int window_width = (int)state.camera.frame_width;

    auto const cleanup = [&]() 
    {
        close_camera(state.camera);
        close_device_list(state.device_list);

        destroy_image(state.screen_image);
        destroy_window_buffer(window_buffer);
        SDL_Quit();
    };

    if (!init_window_buffer(window_buffer, window_width, window_height, app_title))
    {
        cleanup();
        return 1;
    }

    if (!create_image(state.screen_image, window_width, window_height))
    {
        cleanup();
        return 1;
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
    return 0;
}
```

### Grabbing frames

When the library is up and running, we can request frames from the camera.  Here we get a frame, convert it to RGB, and then convert it to RGBA as we write to the application window.

```cpp
#include <algorithm>


void grab_and_convert_frame(Camera& camera, ImageRGBA const& image)
{
    uvc_frame_t* in;

    auto res = uvc_stream_get_frame(camera.h_stream, &in, 0);
    if (res != UVC_SUCCESS)
    {
        uvc_perror(res, "uvc_stream_get_frame()");
        return;
    }

    res = uvc_any2rgb(in, camera.rgb_frame.uvc_frame);
    if (res != UVC_SUCCESS)
    {
        uvc_perror(res, "uvc_any2rgb()");
        return;
    }

    auto frame_begin = (RGB*)camera.rgb_frame.data;
    auto frame_end = frame_begin + camera.frame_width * camera.frame_height;

    auto image_begin = image.data;

    std::transform(frame_begin, frame_end, image_begin, rgb_to_rgba);
}
```

It would be more efficient to convert the frame directly to RGBA instead of having the intermediate step of converting it to RGB first.  However in this simplified example, we do not know what format the source data will be in.  In order to elimnate the the RGB step, we would need to know what image format the camera provides and have a means of converting it to RGBA.  Otherwise we would need to create our own `any2rgba()` helper function.

Update the event handling to display the webcam frames in the window by pressing the 'C' key;

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

    case SDLK_c:
        state.update_frame = [&]() { grab_and_convert_frame(state.camera, state.screen_image); };
        printf("camera\n");
        break;

    default:
        printf("any key\n");
    }
}
```

Now run the program and your webcam video should be displayed in the window.


### Finished

We had to do a little more work this time but we basically made our own version of `cv::VideoCapture`.   Sometimes it's nice to be able to do things yourself.
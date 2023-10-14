# Basic video rendering - Linux edition
## Replacing OpenCV with Libuvc

The previous post demonstrated how to capture webcam image frames using OpenCV and then render them in a window using SDL2.

If all we are doing is grabbing video frames, then we may not want to have OpenCV as a dependency in our application.  In this post we'll cover using a library called [Libuvc](https://github.com/libuvc/libuvc) as an alternative.

Note: I have not been able to get this to work on Windows.  This example was run on Linux Ubuntu 20.04.


### Libuvc installation/setup

Install libusb

```plaintext
sudo apt-get install libusb-1.0-0-dev
```

Libuvc single header [Libuvc single header](https://github.com/adam-lafontaine/Cpp_Utilities/blob/master/libuvc/libuvc.h)


### List devices

```cpp
#include "libuvc.h";

#include <cstdio>


class DeviceList
{
public:
    context* context = nullptr;
    device** device_list = nullptr;

    int n_devices = 0;
};


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

    if (!list.device_list[0])
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

        device_descriptor* desc;

        res = uvc_get_device_descriptor(list.device_list[i], &desc);
        if (res != UVC_SUCCESS)
        {
            uvc_perror(res, "uvc_get_device_descriptor()");
            continue;
        }

        auto vendor_id = (int)desc->idVendor;
        auto product_id = (int)desc->idProduct;        

        printf("    Vendor ID: 0x%04x\n", vendor_id);
        printf("   Product ID: 0x%04x\n", product_id);
        printf(" Manufacturer: %s\n", desc->manufacturer);
        printf("      Product: %s\n", desc->product);
        printf("Serial Number: %s\n", desc->serialNumber);
        printf("\n");

        uvc_free_device_descriptor(desc);
    }
    
    uvc_exit(list.context);
    return 0;
}
```


```plaintext
g++-11
`pkg-config --libs --cflags libusb-1.0` -ljpeg -pthread
```

### Set device permissions

Create a file

```plaintext
/etc/udev/rules.d/99-uvc.rules
```

For each webcam add the following line

```plaintext
SUBSYSTEMS=="usb", ENV{DEVTYPE}=="usb_device", ATTRS{idVendor}=="XXXX", ATTRS{idProduct}=="YYYY", MODE="0666"
```
Replace XXXX and YYYY for the 4 hexadecimal characters corresponding to the vendor and product ID of your webcams.  Restart the computer for the changes to take effect.

We can rewrite the code to just print the information to the console in case we need it.

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

    if (!list.device_list[0])
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

    for (int i = 0; i < list.n_devices; i++)
    {
        device_descriptor* desc;

        auto res = uvc_get_device_descriptor(list.device_list[i], &desc);
        if (res != UVC_SUCCESS)
        {
            uvc_perror(res, "uvc_get_device_descriptor()");
            continue;
        }

        auto vendor_id = (int)desc->idVendor;
        auto product_id = (int)desc->idProduct;
        vp.push_back({ vendor_id, product_id });

        printf("    Vendor ID: 0x%04x\n", vendor_id);
        printf("   Product ID: 0x%04x\n", product_id);
        printf(" Manufacturer: %s\n", desc->manufacturer);
        printf("      Product: %s\n", desc->product);
        printf("Serial Number: %s\n", desc->serialNumber);
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

    if (!find_device_list(list))
    {
        return 1;
    }

    print_device_list(list);

    return 0;
}
```


### SDL2 Recap

The supporting code for SDL2 is identical to that of the [previous post](https://almostalwaysauto.com/posts/basic-video). We'll start at the point where we can set a callback function that updates the image on screen.

Our application state holds the 

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

And our main function is setup to display a 640 x 480 window to start.

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


### Camera device

```cpp

```

```cpp
class Camera
{
public:

};


```



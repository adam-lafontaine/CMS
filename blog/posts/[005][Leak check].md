# No leaks allowed
## How to check for memory leaks

### What is a memory leak?

A memory leak is when memory is given to a program by the operating system and is never returned.  Some programming languages have a garbage collection mechanism which takes care of memory cleanup in the background.  C and C++ do not have this.  Garbage collection comes with a performace cost that many use cases cannot afford.  As a result, it is the responsibility of developers to ensure proper cleanup.  Here is a basic example of how a memory leak can happen.  

```cpp
void memory_leak()
{
    auto total_bytes = 100;

    auto p = malloc(total_bytes);
}
```

Calling malloc() is how a program requests memory from the operating system.  The number of bytes requested is passed to the function and it returns the address of where the memory begins.  The above code is said to leak because the memory is never released back to the operating system.  To release the memory, a corresponding call to free() must be made.

```cpp
void no_memory_leak()
{
    auto total_bytes = 100;

    auto p = malloc(total_bytes);

    free(p);
}
```

Memory leaks become a problem when they cause a program's memory usage to increase over time.  Calls to malloc() get made repeatedly and some or many of the required calls to free() never get made.  The severity of the problem varies.  Sometimes an application will crash, especially if the device has limited memory.  Other times nothing happens because the leak is very small or the program never runs long enough to experience a problem.

Leaks are more likely to occur when a program is complicated and there is no clear owner of the memory.  As pointers are shared between different parts of a program, it's easy to lose track of when it was created and when it should be destroyed.  In Object Oriented Programming, memory is usually allocated when an object is created and destroyed when the object goes out of scope.  This practice is leak-safe and is easier to manage but could have performance implications.  Calls to malloc() and free() are very time consuming in relative terms.  If many objects are being created and destroyed quite often, it could be a bottleneck.  Whatever the coding style, leak-free programming is possible but leaks still end up in finished products all the time.

### Testing for memory leaks

There are tools to help us ensure that we are handling the program's memory properly.  One such tool is Visual Studio on Windows.  The Windows API function _CrtSetDbgFlag() function allows for detecting and reporting memory leaks in a program.  An example of its use is below.

```cpp
#if defined(_WIN32) && defined(_DEBUG)
#define _CRT_DEBUG_MAP_ALLOC
#include <crtdbg.h>
#endif


int main()
{
#if defined(_WIN32) && defined(_DEBUG)
	int dbgFlags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	dbgFlags |= _CRTDBG_CHECK_ALWAYS_DF;   // check block integrity
	dbgFlags |= _CRTDBG_DELAY_FREE_MEM_DF; // don't recycle memory
	dbgFlags |= _CRTDBG_LEAK_CHECK_DF;     // leak report on exit
	_CrtSetDbgFlag(dbgFlags);
#endif

    memory_leak();
}
```

Note:  I have no idea how this actually works.  My professor in college gave us this code and said he'd deduct 50% from our assignements if any leaks were found.

If the program exits without freeing all of its memory, a report is generated in the debug console.

![alt text](https://github.com/adam-lafontaine/CMS/raw/current/blog/img/%5B005%5D/leak_report.png)

Enabling this in a full application will slow it down considerably.  It's best to use a check like this when testing each feature individually.  Memory leak bugs are very hard to track down.  Catching them early in isolated situations prevents many problems down the road.


### A modern example - OpenCV

Yes, OpenCV has memory leaks.  When I was first interested in computer vision, I started with the popular choice.  After writing a small test program, I checked for leaks as is my usual custom.  In order to isolate the problem, I kept removing code until all that was left was a cv::Mat object, the container used for handling an image.

```cpp
#include <opencv2/core.hpp>

void opencv_memory_leak()
{
    cv::Mat img;

    img.release();
}
```

Even with the call to release(), the debug report is very clear.

![alt text](https://github.com/adam-lafontaine/CMS/raw/current/blog/img/%5B005%5D/opencv_leak.png)

So OpenCV does not clean up after itself.  Tsk tsk.  Memory leaks do not always cause a program to crash though.  OpenCV is a good example.  The library is over 20 years old and has been widely used without issue.  However that does not mean that there are no issues.  One consequence is that any application using OpenCV cannot be reliably tested for leaks.  The test will always show leaks regardless.


### Clean code

Writing "clean code" is hard and there is no consensus as to what "clean code" actually is.  Some libraries are leaky and work great.  Others take a very disciplined approach to memory management, yet still don't work or are just not practical to use.  The great thing about software is everything can be solved with typing.  So when we're not happy with an application or library, we can just write it ourselves and learn how difficult it actually is.
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

exe_name := hello_earth

# the html output location
program_exe := $(web)/$(exe_name).html


main_c       := $(code)/main.cpp
main_o       := $(build)/main.o
object_files := $(main_o)


$(main_o): $(main_c)
	@echo "\n main"
	$(EPP) -o $@ -c $<

$(program_exe): $(object_files)
	@echo "\n $(exe_name)"
	$(EPP) -o $@ $+


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




Serve the html file

```plaintext
$ cd {your path}/web/
$ python3 -m http.server 8080
Serving HTTP on 0.0.0.0 port 8080 (http://0.0.0.0:8080/) ...
```

In the browser navigate to localhost:8080/hello_earth.html

![alt text](https://github.com/adam-lafontaine/CMS/raw/current/blog/img/%5B015%5D/em_html.png)
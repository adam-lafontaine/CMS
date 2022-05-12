# Makefiles
## Automating your build system (sort of)

```makefile
do_command:
	@echo "execute something"
	@echo "execute something else"
```

```plaintext
$ make do_command
execute something
execute something else
```


### Tab, not spaces

```plaintext
$ make do_command
Makefile:37: *** missing separator.  Stop.
```

Add a rule to create a build directory if it does not already exist
```makefile
setup:
	mkdir -p ./build
```

Another rule to delete all of the files in the build directory
```makefile
clean:
	rm -rfv ./build/*
```

Create a directory called "code" and we'll start with a main.cpp

```cpp
// ./code/main.cpp
#include <cstdio>


int main()
{
    printf("\n**********************\n");
    printf("*                    *\n");
    printf("*  Hello from main!  *\n");
    printf("*                    *\n");
    printf("**********************\n\n");
}
```

Add a rule to compile the program and another to run it.

```makefile
compile:
	g++ -o ./build/my_program ./code/main.cpp

run:
	./build/my_program
```


Create some variables.

```makefile
build := ./build
code  := ./code

GPP   := g++

exe_name := my_program

program_exe := $(build)/$(exe_name)

main_c := $(code)/main.cpp

setup:
	mkdir -p $(build)

clean:
	rm -rfv $(build)/*

compile:
	$(GPP) -o $(program_exe) $(main_c)

run:	
	$(program_exe)
```

Run the setup rule to create the build directory.

```plaintext
$ make setup
mkdir -p ./build
```

Compile the program

```plaintext
$ make compile
g++ -o ./build/my_program ./code/main.cpp
```

Run the program:

```plaintext
$ make run
./build/my_program

**********************
*                    *
*  Hello from main!  *
*                    *
**********************
```

### More files

math.hpp

```cpp
int add(int a, int b);

int subtract(int a, int b);
```

add.cpp

```cpp
int add(int a, int b)
{
    return a + b;
}
```

subtract.cpp

```cpp
int subtract(int a, int b)
{
    return a - b;
}
```

main.cpp

```cpp
#include "math.hpp"

#include <cstdio>


int main()
{
    printf("\n**********************\n");
    printf("*                    *\n");
    printf("*  Hello from main!  *\n");
    printf("*                    *\n");
    printf("**********************\n\n");

    int a = 2;
    int b = 3;

    printf("%d plus %d equals %d\n", a, b, add(a, b));
    printf("%d minus %d equals %d\n", a, b, subtract(a, b));
}
```

Build an object file for each .cpp file and link them to create an executable


```makefile
build := ./build
code  := ./code
 
exe_name := my_program

program_exe := $(build)/$(exe_name)

main_c := $(code)/main.cpp


$(program_exe):
	g++ -o $(program_exe) $(main_c)

run:	
	$(program_exe)
    @echo "\n"

setup:
	mkdir -p $(build)

clean:
	rm -rfv $(build)/*
```


```makefile
GPP   := g++-10 -std=c++17
build := ./build
code  := ./code

exe_name := my_program

program_exe := $(build)/$(exe_name)

main_c         := $(code)/main.cpp
main_o         := $(build)/main.o

$(main_o): $(main_c) $(math_h)
	@echo "\n main.o"
	$(GPP) -o $@ -c $<


setup:
	mkdir -p $(build)

clean:
	rm -rfv $(build)/*
```



```makefile
GPP   := g++-10 -std=c++17
build := ./build
code  := ./code

exe_name := my_program

program_exe := $(build)/$(exe_name)

main_c         := $(code)/main.cpp
main_o         := $(build)/main.o
object_files   := $(main_o)

math_h := $(code)/math.hpp

add_c        := $(code)/add.cpp
add_o        := $(code)/add.o
object_files += $(add_o)

subtract_c   := $(code)/subtract.cpp
subtract_o   := $(build)/subtract.o
object_files += $(subtract_o)


$(main_o): $(main_c) $(math_h)
	@echo "\n main.o"
	$(GPP) -o $@ -c $<

$(add_o): $(add_c)
	@echo "\n add.o"
	$(GPP) -o $@ -c $<

$(subtract_o): $(subtract_c)
	@echo "\n subtract.o"
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

clean:
	rm -rfv $(build)/*
```


```plaintext
$ make build

 main.o
g++-10 -std=c++17 -o build/main.o -c code/main.cpp

 subtract.o
g++-10 -std=c++17 -o build/subtract.o -c code/subtract.cpp

 my_program
g++-10 -std=c++17 -o build/my_program build/main.o code/add.o build/subtract.o
```


```plaintext
$ make run
./build/my_program

**********************
*                    *
*  Hello from main!  *
*                    *
**********************

2 plus 3 equals 5
2 minus 3 equals -1
```
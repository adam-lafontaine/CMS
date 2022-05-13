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

	printf("Done.\n");
}
```

Add a rule to compile the program and another to run it.

```makefile
compile:
	g++ -o ./build/my_program ./code/main.cpp

run:
	./build/my_program
```

The Makefile so far

```makefile
compile:
	g++ -o ./build/my_program ./code/main.cpp

run:
	./build/my_program

setup:
	mkdir -p ./build

clean:
	rm -rfv ./build/*
```

Run the setup rule to create the build directory with "make setup".

```plaintext
$ make setup
mkdir -p ./build
```

Compile the program with "make compile"

```plaintext
$ make compile
g++ -o ./build/my_program ./code/main.cpp
```

Run the program with "make run"

```plaintext
$ make run
./build/my_program

**********************
*                    *
*  Hello from main!  *
*                    *
**********************

Done.

```

Clean the build directory with "make clean".

```plaintext
$ make clean
rm -rfv ./build/*
removed './build/my_program'
```

### Variables

Create some variables.

```makefile
build := ./build
code  := ./code

GPP   := g++

exe_name := my_program

program_exe := $(build)/$(exe_name)

main_c := $(code)/main.cpp


compile:
	$(GPP) -o $(program_exe) $(main_c)

run:	
	$(program_exe)

setup:
	mkdir -p $(build)

clean:
	rm -rfv $(build)/*
```

Compile the program again.

```plaintext
$ make compile
g++ -o ./build/my_program ./code/main.cpp
```

Run the program.

```plaintext
$ make run
./build/my_program

**********************
*                    *
*  Hello from main!  *
*                    *
**********************

Done.

```


### Dependencies and Automatic Variables

Compile main to an object file and then link to make the executable.

To compile main.cpp into an object file.

```plaintext
g++ -o build/main.o -c code/main.cpp
```

Create a rule using the variables we created

```makefile
$(main_o): $(main_c)
	@echo "\n main"
	$(GPP) -o $(main_o) -c $(main_c)
```

Here main.o is the target and main.cpp is the dependency.  When the target is called, the commands will only execute if there is a change in the dependency.

```makefile
# '$@' is the file name for the target rule
# '$<' is the first prerequisite

$(main_o): $(main_c)
	@echo "\n main"
	$(GPP) -o $@ -c $<
```

To link the object file and build the executable

```plaintext
g++ -o build/my_program build/main.o
```

 The executable depends on main.o.  There are usually multiple object files when linking, so we'll use another automatic variable for the entire list of dependencies.

```makefile
# '$+' is the list of prerequisites

$(program_exe): $(main_o)
	@echo "\n $(exe_name)"
	$(GPP) -o $@ $+
```

Create a build rule that runs the rule for the executable.

```makefile
build: $(program_exe)
```

Change the run rule so that it depends on the build rule before executing the program.

```makefile
run: build
	$(program_exe)
	@echo "\n"
```

We can remove the compile rule as we are now building and linking object files.

```makefile
build := ./build
code  := ./code

GPP   := g++

exe_name := my_program

program_exe := $(build)/$(exe_name)

main_c := $(code)/main.cpp
main_o := $(build)/main.o


$(main_o): $(main_c)
	@echo "\n main"
	$(GPP) -o $@ -c $<

$(program_exe): $(main_o)
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

Now main.o is only recompiled if there is a change in in main.cpp.  The executable only rebuilds if there is a change in main.o.

Clean and then run.

```plaintext
$ make clean
rm -rfv ./build/*
removed './build/my_program'
$ make run

 main
g++ -o build/main.o -c code/main.cpp

 my_program
g++ -o build/my_program build/main.o
./build/my_program

**********************
*                    *
*  Hello from main!  *
*                    *
**********************

Done.

```

Everything was compiled and linked before running the program.

Run again

```plaintext
$ make run
./build/my_program

**********************
*                    *
*  Hello from main!  *
*                    *
**********************

Done.

```

No changes were detected in main.cpp so only the existing executable was run.

Make change to main.cpp and run again

```plaintext
$ make run

 main
g++ -o build/main.o -c code/main.cpp

 my_program
g++ -o build/my_program build/main.o
./build/my_program

**********************
*                    *
*  Hello again from main!  *
*                    *
**********************

Done.

```

Since main.cpp was changed, everything had to be recompiled.

### More files

In the code directory create math.hpp

```cpp
int add(int a, int b);

int subtract(int a, int b);
```

Create and implementation file for each function.

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

Update main.cpp to include the new functions.

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

    printf("Done.\n");
    
}
```

Create variables for the new files.

```makefile
math_h      := $(code)/math.hpp

add_c       := $(code)/add.cpp
add_o       := $(code)/add.o

subtract_c  := $(code)/subtract.cpp
subtract_o  := $(build)/subtract.o
```

Make rules for the new implementation files in the same fashion as main.cpp.

```makefile
$(add_o): $(add_c)
	@echo "\n add"
	$(GPP) -o $@ -c $<

$(subtract_o): $(subtract_c)
	@echo "\n subtract"
	$(GPP) -o $@ -c $<
```

Since main.cpp now includes math.hpp its rule must be updated to reflect the dependency.

```makefile
$(main_o): $(main_c) $(math_h)
	@echo "\n main"
	$(GPP) -o $@ -c $<
```

Create a variable to store the list of object files.

```makefile
object_files := $(main_o)
object_files += $(add_o)
object_files += $(subtract_o)
```

Update the rule for linking the program by having it depend on and link all the the object files.

```makefile
$(program_exe): $(object_files)
	@echo "\n $(exe_name)"
	$(GPP) -o $@ $+
```

The final version of our makefile.

```makefile
GPP   := g++
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
	@echo "\n main"
	$(GPP) -o $@ -c $<

$(add_o): $(add_c)
	@echo "\n add"
	$(GPP) -o $@ -c $<

$(subtract_o): $(subtract_c)
	@echo "\n subtract"
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

Run the new program

```plaintext
$ make run

 main
g++ -o build/main.o -c code/main.cpp

 subtract
g++ -o build/subtract.o -c code/subtract.cpp

 my_program
g++ -o build/my_program build/main.o code/add.o build/subtract.o
./build/my_program

**********************
*                    *
*  Hello from main!  *
*                    *
**********************

2 plus 3 equals 5
2 minus 3 equals -1
Done.

```

Now edit add.cpp so that the function multiplies the numbers instead of adding them.

Run again.

```plaintext
$ make run

 add
g++ -o code/add.o -c code/add.cpp

 my_program
g++ -o build/my_program build/main.o code/add.o build/subtract.o
./build/my_program

**********************
*                    *
*  Hello from main!  *
*                    *
**********************

2 plus 3 equals 6
2 minus 3 equals -1
Done.

```

Only add.cpp was recompiled.  
# Makefiles
## Automating your build system (sort of)

Make is a tool for automating shell commands, and is primarily used for building C and C++ programs.  It allows for compiling only parts of a program depending on which files have changed since the last build.  If your project is small, then recompiling everything is usually good enough.  For large projects however, recompiling can take several minutes after making only a small change.  A well-made makefile can significantly reduce build times freeing up more time for actual work.

We will cover some of the basics of makefiles while creating a template to use for future projects.  The examples in this post are done on Linux.  Make is also available for Windows but Visual Studio is much easier to use.

### How To

At minimum, make allows for creating a shortcut for one or more shell commands.  To start, create a file and give it the name "Makefile".  Paste the following inside of it.

```makefile
do_command:
	@echo "execute something"
	@echo "execute something else"
```

Open a terminal in that directory and type "make do_command".

```plaintext
$ make do_command
execute something
execute something else
```

The two echo commands where executed.  The @ at the beginning prevents the shell command from being printed to the terminal before executing it.

We created what is called a rule.  The target is do_command and is followed by a colon.  The commands for the rule are indented below it.

### Tab, not spaces

The indent for the commands must a tab and not spaces.  Some editors are set to use spaces when the tab key is pressed.  If this is the case, you'll get an error like this.

```plaintext
$ make do_command
Makefile:37: *** missing separator.  Stop.
```

### New Project

When starting a new project, it's good to have a separate directory for all of the build files.  Usually, the build directory is ignored in git repositories.  I like to make a setup rule for myself or anyone else who clones the repository on another device.

Add a rule to create a build directory if it does not already exist.

```makefile
setup:
	mkdir -p ./build
```

Every now and then we will want to clear the build directory and rebuild from scratch.  Add another rule to delete the files.

```makefile
clean:
	rm -rfv ./build/*
```

Create a directory called "code" and we'll start with main.cpp.

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
``

So far our makefile has four rules.

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

Run the setup rule to create the build directory by typing "make setup" in the terminal.

```plaintext
$ make setup
mkdir -p ./build
```

Compile the program with "make compile".

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

Make allows for using variables.  They are good for directories and file paths that are referenced multiple times.  Adding some variables makes our makefile look like so.

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

If you didn't get any errors then your variables have been set and referenced properly.


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

Only add.cpp was recompiled.  The other implementation files, subtract.cpp and main.cpp were left alone.
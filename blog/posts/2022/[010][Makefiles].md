# Makefiles
## Automating your build system (sort of)

Make is a tool for automating shell commands, and is primarily used for building C and C++ programs and libraries.  It allows for compiling only parts of a program at a time, depending on which files have changed since the last build.  If your project is small, then recompiling everything together is usually good enough.  For large projects however, recompiling can take several minutes after making only a small change.  A well made makefile can significantly reduce build times freeing up more time for actual work.

We will cover some of the basics of makefiles while creating a template to use for future projects.  The examples in this post are done on Linux.  Make is also available for Windows with some extra effort to get it installed.

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

The two echo commands where executed.  The @ at the beginning prevents the shell command from being printed to the terminal before executing it.  Without the @, the output would look like this.

```plaintext
$ make do_command
echo "execute something"
execute something
echo "execute something else"
execute something else
```

We created what is called a rule.  The target is "do_command" and is followed by a colon.  The commands for the rule are indented below it.

### Tab, not spaces

The indent for the commands must be a tab and not spaces.  Some editors are set to use spaces when the tab key is pressed.  If this is the case, you'll get an error like this.

```plaintext
$ make do_command
Makefile:37: *** missing separator.  Stop.
```

You will need to change your editor's tab settings to fix this.

### New Project

When starting a new project, it's good to have a separate directory for all of the build files.  The build directory is usually ignored in git repositories so I like to make a setup rule for myself or anyone else who clones the repository on another device.

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

Create a directory called "code" and we'll start by adding main.cpp to it.

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

So far our makefile has four rules: compile, run, setup, and clean.

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

To demonstrate other capabilities of Make, we'll compile main to an object file and then link it to make the executable instead of doing it all in one compile step.

Here is the command to compile main.cpp into an object file.

```plaintext
g++ -o build/main.o -c code/main.cpp
```

In the makefile, create variable for the object file and a rule using the variables we created

```makefile
main_o := $(build)/main.o


$(main_o): $(main_c)
	@echo "\n main"
	$(GPP) -o $(main_o) -c $(main_c)
```

Here main.o is the target and main.cpp is the dependency.  The compile command will only execute if a change was made to main.cpp.

A rule can have multiple dependencies and they are listed to the right of the colon.

For convenience, Make has some automatic variables so that we don't have to make sure that the files referenced in the compilation command match those in the rule. \$@ returns the filename for the target.  \$< returns the first filename in a list of dependencies.  It allows for adding additional dependencies without adding them to the command.

Our rule for main.o can be updated to the following.

```makefile
# $@ returns the filename for the target
# $< returns the first dependency

$(main_o): $(main_c)
	@echo "\n main"
	$(GPP) -o $@ -c $<
```

Next we want to link the object file and build the executable.

```plaintext
g++ -o build/my_program build/main.o
```

 The executable depends on main.o.  There are usually multiple object files when linking, so we'll use another automatic variable that returns the entire list of dependencies (\$+).  This way we can add object files as dependencies without having to modify the command.

 Create a rule for the executable itself that performs the final linking step.

```makefile
# '$+' returns the entire list of dependencies

$(program_exe): $(main_o)
	@echo "\n $(exe_name)"
	$(GPP) -o $@ $+
```

We can create rules that run other rules.  This is useful when we want a commonly named rule for files that may change often or be different across projects.

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

Since we are now building and linking object files, we no longer need the compile rule from earlier.  After removing it, our makefile for compiling and linking a single file is below.

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
	@echo "\n"

clean:
	rm -rfv $(build)/*
	@echo "\n"
```

Now main.o is only recompiled if there is a change in in main.cpp.  The executable only rebuilds if there is a change in main.o.

To test it out, first clean and run.

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

Run it again.

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

Make a change to main.cpp and run once more.

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

### Adding Files

As a software project grows, more files will be added.  We'll add some files and see how we can handle this  in our makefile.

In the code directory create a header file with the following function definitions.

math.hpp 

```cpp
int add(int a, int b);

int subtract(int a, int b);
```

For demonstration purposes, create a separate implementation file for each function.

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

We added a header file and two implementation files.  Each implementation file will need to be compiled into its own object file, and they will all need to be linked to build the executable.

In the makefile, create variables for the new files.

```makefile
math_h      := $(code)/math.hpp

add_c       := $(code)/add.cpp
add_o       := $(build)/add.o

subtract_c  := $(code)/subtract.cpp
subtract_o  := $(build)/subtract.o
```

Add rules for the new implementation files in the same fashion as main.cpp.

```makefile
$(add_o): $(add_c)
	@echo "\n add"
	$(GPP) -o $@ -c $<

$(subtract_o): $(subtract_c)
	@echo "\n subtract"
	$(GPP) -o $@ -c $<
```

Since main.cpp now includes math.hpp, its rule must be updated to reflect the dependency.

```makefile
$(main_o): $(main_c) $(math_h)
	@echo "\n main"
	$(GPP) -o $@ -c $<
```

Note: \$< will only return the first file in the list of dependencies so math.hpp will not be included in the compilation command.

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

Note: \$+ ensures that all of the dependencies in the object_files variable will be included in the command.

After making these changes, this is what the final version of our makefile looks like.

```makefile
GPP   := g++
build := ./build
code  := ./code

exe_name := my_program

program_exe := $(build)/$(exe_name)

math_h := $(code)/math.hpp

main_c       := $(code)/main.cpp
main_o       := $(build)/main.o
object_files := $(main_o)

add_c        := $(code)/add.cpp
add_o        := $(build)/add.o
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
	@echo "\n"

clean:
	rm -rfv $(build)/*
	@echo "\n"
```

Running the new program will cause everything to be compiled.

```plaintext
$ make run

 main
g++ -o build/main.o -c code/main.cpp

 add
g++ -o build/add.o -c code/add.cpp

 subtract
g++ -o build/subtract.o -c code/subtract.cpp

 my_program
g++ -o build/my_program build/main.o build/add.o build/subtract.o
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

```cpp
int add(int a, int b)
{
    return a * b;
}
```

Run again.

```plaintext
$ make run

 add
g++ -o build/add.o -c code/add.cpp

 my_program
g++ -o build/my_program build/main.o build/add.o build/subtract.o
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

We can see that only add.cpp was recompiled.  The other object files, subtract.o and main.o were left alone.

### Your Build System

This is the extent to which I use Make.  The system follows a pattern that makes adding new files to a project pretty straight forward.

Starting with something like the makefile we made here, we can add new variables for files as needed.

```makefile
some_header_h := $(code)/some_header.hpp

some_file_c  := $(code)/some_file.cpp
some_file_o  := $(build)/some_file.o
object_files += $(some_file_o)
```

And then add a new rule for each implementation file making sure to include the correct dependencies.

```makefile
$(some_file_o): $(some_file_c) $(some_header_h)
	@echo "\n some_file"
	$(GPP) -o $@ -c $<
```

The rest will take care of itself with "make build" or "make run".  

There is much more to Make then what was shown here.  Learning it will be time well spent and no doubt there are improvements that can be made to the build system here.  Remember that the point of a build system is to free up time for software development.  It can be tempting to get caught up on making the ultimate build system and neglect the project that you are actually working on.  Use your judgement.
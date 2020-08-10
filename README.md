# Heaven

This is a collection of utilities to manage projects in a unix-like fashion.
It contains:

- hvn : Frontend to other subcommands.
- hvn-cfgen : Create a basic configure script, which you can then modify manually to suit your needs.
- hvn-init : Add a new module to your sources, and creates a hello world if the language is supported.
- hvn-mando : Create roff manpages from markdown. Requires libcmark.
- hvn-mkgen : Create a Makefile.rules containing rules to create object files associated to filename's extensions.
- hvn-pkgen : Execute a set of scripts inside an isolated environment.

## Why?

Usually, projects tend to use tools such as cmake, ninja or whatever to create Makefiles when cloning a project.
The goal was to be able to minimize the work of a cloner who just wants to build things.
To respect this goal, I had to create utilities which only created files for tools everybody have: shell and make.

## How to use it?

Let's say we want to create a `libfoo` we use in an executable named `bar`, all written in C.
Basically, you create a project, you just mkdir in one place, maybe git init, and then:

	hvn-init -e .c libfoo bar

Here, we tell `hvn-init` to create two modules, libfoo will be a library because of its prefix.
And bar an executable because it doesn't have the lib prefix. All in the default sources location, `src/`
We specify `.c` for the main file created in executables modules.

Ok, now let's say you've coded a few things in `src/libfoo/` and `src/bar/`, you want to build that. First, create a configuration file:

	hvn-cfgen -lC

You now have a `configure` shell script in your current directory.
You can add custom linking rules at the macro created exclusively for each module, I'll let you read the script to tweak it.
The script is meant to take a `Makefile.rules` skeleton to generate a `Makefile`.
The `-lC` option tells you want the script to look up for a CC compiler options. You can also ask for `C++` or both if you wanted.

Then, we create the `Makefile.rules` using `hvn-mkgen`:

	hvn-mkgen -dbar=libfoo

The `-d` option specifies a list of comma separated dependencies for a module.

Now, you can:

	./configure

Which will create `build/{bin,lib}` directories and a `Makefile`. In theory you should be able to run `make` just after that.
But actually, under GNU/Linux, the linker doesn't look up for the crt's and compiler runtime. A fast workaround is to specify the compiler as the linker.
The compiler-driver linker will certainly do the job:

	LD=clang ./configure

Then, you can `make` your project and you execute rules based on objects derived from your source tree.
Try not to name two potentials object with the same basename, as the `.o` prefix __replaces__ the source file one.

Yet the only one exempted from examples is certainly the most powerful: `hvn-pkgen`.
This utility is linux-specific and enables creating containers to create a standard environment for executing/building/packaging.
The main goal is to fake an environment where each build are ran in userspace but hiding the system headers and interfaces.
It allows the package builder to easily change a toolchain or build scripts for distribution.
I'll let you read the manual pages to understand how you can use it. You're only limited by your imagination.

## Build

Well, I'll let you figure it out from the previous section.

## Install

For this, you can manually copy the hierarchy under `build/` whenever your environment likes it.


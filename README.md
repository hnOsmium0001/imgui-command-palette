# ImGui Command Palette

## About
This library implements a Sublime Text or VSCode style command palette in ImGui.
It is provided in the form of a window that you can choose to open/close based on condition (for example, when user pressed the shortcut Ctrl+Shift+P).

## Features
+ Dynamic registration and unregistration of commands
+ Subcommands (prompting a new set of options after user selected a top-level command)
+ Fuzzy search of commands and subcommands
    + Highlighting of matched characters (requires setting a separate font)

## Planned Features
+ [ ] Support for std::string_view
+ [ ] Support for function pointers instead of std::function
+ [ ] Visualization of previously entered options (example: Sublime Merge)
+ [ ] Highlighting of matched characters using unerline
+ [ ] Command history
+ [ ] Reducing the minimum required C++ version

## Usage
Simply drop all .h and .cpp files in the project root folder to your buildsystem. Minimum of C++17 is required.
No external dependencies except Dear ImGui are required.

See the examples for how to use the APIs.

## Demo
This project provides examples located in the `examples/` folder. All dependencies are retreived with conan and built with CMake.
One option to build the examples:
```
# In examples/
$ mkdir build
$ cd build
$ conan install .. -s build_type=Debug
$ cmake .. -GNinja
$ ninja
```

Note: run the examples with working directory = `${projectFolder}/examples`, since they use relative paths to locate the fonts

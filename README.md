# ImGui Command Palette

![screenshot1](https://user-images.githubusercontent.com/36975818/146656302-646eccfd-6bf4-4ad0-80e0-239c7766210a.png)

## About
This library implements a Sublime Text or VSCode style command palette in ImGui.
It is provided in the form of a window that you can choose to open/close based on condition (for example, when user pressed the shortcut Ctrl+Shift+P).

## Features
+ Minimum C++ 11
+ Dynamic registration and unregistration of commands
+ Subcommands (prompting a new set of options after user selected a top-level command)
+ Fuzzy search of commands and subcommands
    + Highlighting of matched characters
        + Option: setting custom font
        + Option: setting custom text color


## Planned Features
+ [ ] Support for std::string_view
+ [ ] Support for function pointers instead of std::function
+ [ ] Visualization of previously entered options (example: Sublime Merge)
+ [x] Highlighting of matched characters using underline
+ [ ] Command history
+ [x] Reducing the minimum required C++ version

## Usage
Simply drop all .h and .cpp files in the project root folder to your buildsystem. Minimum of C++11 is required.
No external dependencies except Dear ImGui are required.

See the examples for how to use the APIs.

## Examples
The `examples/` folder is organized in the following way:
- The `src/` and `fonts/` folders contains source code and assets for the example app. If you want to hack away, the relevant code are all here!
- The `app-*/` folders each provide a separate way to build the project. Choose one that works the best for you -- they should all produce the same result.
    - `app-vcpkg/` provides CMake + vcpkg build scripts
        ```sh
        $ pwd
        <project folder>/examples/app-vcpkg
        $ mkdir build
        $ cd build
        $ cmake .. -GNinja -DCMAKE_TOOLCHAIN_FILE="/path/to/your-vcpkg/scripts/buildsystem/vcpkg.cmake"
        $ ninja
        ```
    - `app-manual/` provides CMake + system deps + file drop in
        - Ensure `glfw3` is installed as a system package, available for CMakle `find_package`
        - Download the following files from https://github.com/ocornut/imgui and drop them in `app-manual/`
            - Every .h and .cpp in the root dir (that is, imgui itself)
            - `backends/imgui_impl_glfw.h`
            - `backends/imgui_impl_glfw.cpp`
            - `backends/imgui_impl_opengl3.h`
            - `backends/imgui_impl_opengl3.cpp`
            - `backends/imgui_impl_opengl3_loader.h`
        ```sh
        $ pwd
        <project folder>/examples/app-manual
        $ mkdir build
        $ cd build
        $ cmake .. -GNinja
        $ ninja
        ```

Once you have it built, run the examples with working directory = `examples/`.
This is needed because the app uses relative paths to locate fonts.
That is, if the build had produced `examples/app-vcpkg/build/imcmd-demo.exe` for example, you should run it as:
```sh
$ pwd
<project folder>/examples
$ ./app-vcpkg/build/build/imcmd-demo.exe ⏎
```

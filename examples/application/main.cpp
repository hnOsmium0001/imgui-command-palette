#include "imcmd_command_palette.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glad/glad.h>
#include <imgui.h>
#include <iostream>
#include <string>

#include <../res/bindings/imgui_impl_glfw.h>
#include <../res/bindings/imgui_impl_opengl3.h>

#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#include <../res/bindings/imgui_impl_glfw.cpp>
#include <../res/bindings/imgui_impl_opengl3.cpp>

static void GlfwErrorCallback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

int main()
{
    if (!glfwInit()) {
        return -1;
    }

    glfwSetErrorCallback(&GlfwErrorCallback);

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

    GLFWwindow* window = glfwCreateWindow(1280, 720, "ImGui Command Palette Example", nullptr, nullptr);
    if (window == nullptr) {
        return -2;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        return -3;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    auto& io = ImGui::GetIO();
    auto regular_font = io.Fonts->AddFontFromFileTTF("fonts/NotoSans-Regular.ttf", 16, nullptr, io.Fonts->GetGlyphRangesDefault());
    auto bold_font = io.Fonts->AddFontFromFileTTF("fonts/NotoSans-Bold.ttf", 16, nullptr, io.Fonts->GetGlyphRangesDefault());

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    ImGuiCommandPalette::CommandRegistry command_registry;
    ImGuiCommandPalette::CommandPalette command_palette(command_registry);

    command_palette.RegularFont = regular_font;
    command_palette.HighlightFont = bold_font;

    bool show_demo_window = true;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    using ImGuiCommandPalette::Command;
    using ImGuiCommandPalette::CommandExecutionContext;
    using namespace std::literals::string_literals;
    command_registry.AddCommand(Command{
        .Name = "Toggle ImGui demo window",
        .InitialCallback = [&](CommandExecutionContext& ctx) -> void {
            show_demo_window = !show_demo_window;
            ctx.Finish();
        },
    });
    command_registry.AddCommand(Command{
        .Name = "Select theme",
        .InitialCallback = [&](CommandExecutionContext& ctx) -> void {
            ctx.Prompt({
                "Classic"s,
                "Dark"s,
                "Light"s,
            });
        },
        .SubsequentCallback = [&](CommandExecutionContext& ctx, int selected_option) -> void {
            switch (selected_option) {
                case 0: ImGui::StyleColorsClassic(); break;
                case 1: ImGui::StyleColorsDark(); break;
                case 2: ImGui::StyleColorsLight(); break;
                default: break;
            }
            ctx.Finish();
        },
    });

    Command example_command{
        .Name = "Example command",
    };
    command_registry.AddCommand(example_command);
    command_registry.AddCommand(Command{
        .Name = "Add 'Example command'",
        .InitialCallback = [&](CommandExecutionContext& ctx) -> void {
            command_registry.AddCommand(example_command);
            ctx.Finish();
        },
    });
    command_registry.AddCommand(Command{
        .Name = "Remove 'Example command'",
        .InitialCallback = [&](CommandExecutionContext& ctx) -> void {
            command_registry.RemoveCommand(example_command.Name);
            ctx.Finish();
        },
    });

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (show_demo_window) {
            ImGui::ShowDemoWindow(&show_demo_window);
        }

        if (io.KeyCtrl && io.KeyShift && ImGui::IsKeyPressed(GLFW_KEY_P)) {
            bool visible = command_palette.IsVisible();
            command_palette.SetVisible(!visible);
        }
        command_palette.Show("Command Palette");

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

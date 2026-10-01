#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <GLFW/glfw3.h>

#include <filesystem>
#include <iostream>

namespace {

constexpr const char* kGlslVersion = "#version 150";

std::filesystem::path ResourcePath(const char* relative) {
    return std::filesystem::path(BAOJIAOZI_SOURCE_DIR) / relative;
}

bool LoadFonts() {
    ImGuiIO& io = ImGui::GetIO();
    const auto textFont = ResourcePath("resources/fonts/switch_font.ttf");
    const auto iconFont = ResourcePath("resources/fonts/MaterialIcons-Regular.ttf");
    if (!std::filesystem::is_regular_file(textFont) ||
        !std::filesystem::is_regular_file(iconFont)) {
        std::cerr << "Baojiaozi fonts are missing\n";
        return false;
    }

    io.Fonts->AddFontFromFileTTF(textFont.string().c_str(), 18.0f,
                                 nullptr, io.Fonts->GetGlyphRangesChineseFull());
    return true;
}

} // namespace

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(1280, 800, "Baojiaozi Designer", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        std::cerr << "Failed to create the designer window\n";
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(kGlslVersion);
    const bool fontsLoaded = LoadFonts();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Baojiaozi Designer");
        ImGui::TextUnformatted("Phase 0 - project bootstrap");
        ImGui::Text("Version: %s", "0.1.0-dev");
        ImGui::Text("Fonts: %s", fontsLoaded ? "loaded" : "missing");
        ImGui::TextUnformatted("The document runtime will be added in Phase 1.");
        ImGui::End();

        ImGui::Render();
        int displayWidth = 0;
        int displayHeight = 0;
        glfwGetFramebufferSize(window, &displayWidth, &displayHeight);
        glViewport(0, 0, displayWidth, displayHeight);
        glClearColor(0.08f, 0.09f, 0.11f, 1.0f);
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

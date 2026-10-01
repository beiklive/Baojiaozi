#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include "baojiaozi/imgui/renderer.hpp"
#include "baojiaozi/designer/project_store.hpp"
#include "baojiaozi/designer/blueprint.hpp"
#include "baojiaozi/parser/project_loader.hpp"
#include "baojiaozi/runtime/runtime.hpp"

#include <GLFW/glfw3.h>

#include <filesystem>
#include <algorithm>
#include <cstdio>
#include <functional>
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

    ImFont* defaultFont = io.Fonts->AddFontFromFileTTF(
        textFont.string().c_str(), 18.0f, nullptr, io.Fonts->GetGlyphRangesChineseFull());
    if (defaultFont != nullptr) io.FontDefault = defaultFont;
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
    glfwMaximizeWindow(window);

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(kGlslVersion);
    const bool fontsLoaded = LoadFonts();

    baojiaozi::parser::ProjectLoader loader;
    const auto projectResult = loader.LoadDirectory(
        ResourcePath("resources/examples/default_theme"));
    if (!projectResult.Succeeded()) {
        for (const auto& diagnostic : projectResult.diagnostics) {
            std::cerr << diagnostic.source << diagnostic.path << ": " << diagnostic.message << '\n';
        }
    }
    const auto project = projectResult.project;
    auto editableProject = project;
    baojiaozi::imgui::Renderer renderer;
    std::string previewEvent = "normal";
    float previewTime = 0.0f;
    std::string activePage = "home";
    std::string selectedNode;
    std::string saveMessage;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("项目")) {
                if (ImGui::MenuItem("保存")) {
                    if (editableProject) {
                        std::string error;
                        const bool saved = baojiaozi::designer::ProjectStore::SavePage(
                            *editableProject, activePage,
                            ResourcePath("resources/examples/default_theme"), error);
                        saveMessage = saved ? "已保存主页" : error;
                    }
                }
                ImGui::MenuItem("退出", "Cmd+Q");
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("编辑")) {
                ImGui::MenuItem("撤销", "Cmd+Z");
                ImGui::MenuItem("重做", "Cmd+Shift+Z");
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("视图")) ImGui::EndMenu();
            if (ImGui::BeginMenu("设置")) ImGui::EndMenu();
            if (ImGui::BeginMenu("关于")) ImGui::EndMenu();
            ImGui::EndMainMenuBar();
        }

        const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(mainViewport->WorkPos);
        ImGui::SetNextWindowSize(mainViewport->WorkSize);
        constexpr ImGuiWindowFlags fullWindowFlags =
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        ImGui::Begin("Baojiaozi Designer", nullptr, fullWindowFlags);
        ImGui::TextUnformatted("Baojiaozi Default Theme");
        ImGui::Text("Fonts: %s", fontsLoaded ? "loaded" : "missing");
        ImGui::Separator();

        ImGui::BeginChild("asset_panel", ImVec2(220.0f, 0.0f), true);
        ImGui::TextUnformatted("控件列表");
        ImGui::BulletText("Box");
        ImGui::BulletText("Text");
        ImGui::BulletText("Image");
        ImGui::BulletText("Button");
        ImGui::Separator();
        ImGui::TextUnformatted("页面列表");
        if (editableProject) {
            for (const auto& page : editableProject->pages) {
                if (ImGui::Selectable(page.title.c_str(), page.id == activePage)) {
                    activePage = page.id;
                    selectedNode.clear();
                }
            }
        }
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild("workspace_panel", ImVec2(-260.0f, 0.0f), true);
        if (ImGui::BeginTabBar("workspace_tabs")) {
        if (ImGui::BeginTabItem("即时渲染")) {
        ImGui::TextUnformatted("即时渲染预览");
        ImGui::Text("当前页面：%s", activePage.c_str());
        if (ImGui::Button("普通")) {
            previewEvent = "normal";
            previewTime = 0.0f;
        }
        ImGui::SameLine();
        if (ImGui::Button("聚焦")) {
            previewEvent = "focus";
            previewTime = 0.0f;
        }
        ImGui::SameLine();
        if (ImGui::Button("触发")) {
            previewEvent = "trigger";
            previewTime = 0.0f;
        }
        ImGui::SameLine();
        ImGui::Text("事件: %s  时间: %.2fs", previewEvent.c_str(), previewTime);
        ImGui::Separator();
        const ImVec2 previewOrigin = ImGui::GetCursorScreenPos();
        const ImVec2 previewSize = ImGui::GetContentRegionAvail();
        ImGui::InvisibleButton("preview_surface", previewSize);
        if (editableProject) {
            baojiaozi::runtime::Runtime runtime(*editableProject);
            if (previewEvent != "normal") previewTime += ImGui::GetIO().DeltaTime;
            const auto view = runtime.BuildPage(
                activePage, {0.0f, 0.0f, previewSize.x, previewSize.y},
                {previewEvent, previewTime});
            renderer.Render(view.root, previewOrigin);
        }
        ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("蓝图")) {
            ImGui::TextUnformatted("页面节点蓝图");
            if (editableProject) {
                auto pageIt = std::find_if(editableProject->pages.begin(), editableProject->pages.end(),
                                           [&](const auto& page) { return page.id == activePage; });
                if (pageIt != editableProject->pages.end()) {
                    const auto blueprint = baojiaozi::designer::BuildBlueprint(pageIt->root);
                    for (const auto& node : blueprint.nodes) {
                        ImGui::SetCursorPos({20.0f + node.x, 50.0f + node.y});
                        ImGui::Button(node.label.c_str(), {190.0f, 48.0f});
                    }
                    ImGui::SetCursorPos({20.0f, 50.0f + static_cast<float>(blueprint.nodes.size()) * 92.0f});
                    ImGui::Text("连线: %zu", blueprint.links.size());
                    for (const auto& link : blueprint.links) ImGui::BulletText("%s -> %s", link.from.c_str(), link.to.c_str());
                }
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
        }
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild("inspector_panel", ImVec2(0.0f, 0.0f), true);
        ImGui::TextUnformatted("属性检查器");
        ImGui::Text("选中节点: %s", selectedNode.empty() ? "无" : selectedNode.c_str());
        if (editableProject) {
            auto pageIt = std::find_if(editableProject->pages.begin(), editableProject->pages.end(),
                                       [&](const auto& page) { return page.id == activePage; });
            if (pageIt != editableProject->pages.end()) {
                std::function<void(baojiaozi::document::Node&)> drawNode;
                drawNode = [&](baojiaozi::document::Node& node) {
                    if (ImGui::Selectable((node.id + " (" + baojiaozi::document::ToString(node.type) + ")").c_str(),
                                          selectedNode == node.id)) {
                        selectedNode = node.id;
                    }
                    for (auto& child : node.children) drawNode(child);
                };
                drawNode(pageIt->root);

                std::function<baojiaozi::document::Node*(baojiaozi::document::Node&)> findNode;
                findNode = [&](baojiaozi::document::Node& node) -> baojiaozi::document::Node* {
                    if (node.id == selectedNode) return &node;
                    for (auto& child : node.children) if (auto* found = findNode(child)) return found;
                    return nullptr;
                };
                if (auto* node = findNode(pageIt->root)) {
                    ImGui::Separator();
                    char textBuffer[256] = {};
                    const auto currentText = node->properties.value("text", std::string{});
                    std::snprintf(textBuffer, sizeof(textBuffer), "%s", currentText.c_str());
                    if (ImGui::InputText("文本", textBuffer, sizeof(textBuffer))) node->properties["text"] = textBuffer;
                    int fontSize = node->properties.value("fontSize", 18);
                    if (ImGui::InputInt("字号", &fontSize)) node->properties["fontSize"] = fontSize;
                }
            }
        }
        if (!saveMessage.empty()) ImGui::TextWrapped("%s", saveMessage.c_str());
        ImGui::EndChild();
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

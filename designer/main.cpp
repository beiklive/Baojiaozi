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

ImU32 BlueprintNodeColor(baojiaozi::designer::BlueprintNodeKind kind) {
    using baojiaozi::designer::BlueprintNodeKind;
    switch (kind) {
    case BlueprintNodeKind::Control: return IM_COL32(31, 124, 143, 255);
    case BlueprintNodeKind::State: return IM_COL32(116, 82, 156, 255);
    case BlueprintNodeKind::Property: return IM_COL32(155, 91, 39, 255);
    }
    return IM_COL32(80, 86, 96, 255);
}

ImU32 BlueprintLinkColor(baojiaozi::designer::BlueprintLinkKind kind) {
    using baojiaozi::designer::BlueprintLinkKind;
    switch (kind) {
    case BlueprintLinkKind::Child: return IM_COL32(53, 207, 200, 255);
    case BlueprintLinkKind::State: return IM_COL32(186, 119, 233, 255);
    case BlueprintLinkKind::Property: return IM_COL32(242, 174, 71, 255);
    }
    return IM_COL32(220, 224, 232, 255);
}

void DrawBlueprintCanvas(const baojiaozi::designer::BlueprintDocument& blueprint) {
    constexpr float nodeWidth = 220.0f;
    const ImVec2 available = ImGui::GetContentRegionAvail();
    const ImVec2 canvasSize(std::max(available.x, 900.0f), std::max(available.y, 520.0f));
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##blueprint_canvas", canvasSize);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 canvasMax(origin.x + canvasSize.x, origin.y + canvasSize.y);
    draw->AddRectFilled(origin, canvasMax, IM_COL32(17, 23, 31, 255));
    draw->PushClipRect(origin, canvasMax, true);
    for (float x = 0.0f; x < canvasSize.x; x += 24.0f) {
        draw->AddLine({origin.x + x, origin.y}, {origin.x + x, canvasMax.y}, IM_COL32(25, 34, 45, 255));
    }
    for (float y = 0.0f; y < canvasSize.y; y += 24.0f) {
        draw->AddLine({origin.x, origin.y + y}, {canvasMax.x, origin.y + y}, IM_COL32(25, 34, 45, 255));
    }

    auto findNode = [&](const std::string& id) -> const baojiaozi::designer::BlueprintNode* {
        const auto it = std::find_if(blueprint.nodes.begin(), blueprint.nodes.end(),
                                     [&](const auto& node) { return node.id == id; });
        return it == blueprint.nodes.end() ? nullptr : &*it;
    };
    auto nodeHeight = [](const baojiaozi::designer::BlueprintNode& node) {
        return node.kind == baojiaozi::designer::BlueprintNodeKind::Property ? 64.0f : 58.0f;
    };
    for (const auto& link : blueprint.links) {
        const auto* from = findNode(link.from);
        const auto* to = findNode(link.to);
        if (from == nullptr || to == nullptr) continue;
        const ImVec2 a(origin.x + from->x + nodeWidth, origin.y + from->y + nodeHeight(*from) * 0.5f);
        const ImVec2 b(origin.x + to->x, origin.y + to->y + nodeHeight(*to) * 0.5f);
        const float bend = std::max(42.0f, std::abs(b.x - a.x) * 0.45f);
        draw->AddBezierCubic(a, {a.x + bend, a.y}, {b.x - bend, b.y}, b,
                            BlueprintLinkColor(link.kind), 2.5f);
    }

    for (const auto& node : blueprint.nodes) {
        const ImVec2 min(origin.x + node.x, origin.y + node.y);
        const ImVec2 max(min.x + nodeWidth, min.y + nodeHeight(node));
        draw->AddRectFilled(min, max, IM_COL32(26, 33, 44, 255), 6.0f);
        draw->AddRectFilled(min, {max.x, min.y + 32.0f}, BlueprintNodeColor(node.kind), 6.0f);
        draw->AddRectFilled({min.x, min.y + 26.0f}, {max.x, min.y + 32.0f}, BlueprintNodeColor(node.kind));
        draw->AddRect(min, max, IM_COL32(85, 100, 120, 255), 6.0f, 0, 1.0f);
        draw->AddText({min.x + 10.0f, min.y + 8.0f}, IM_COL32(245, 245, 245, 255), node.label.c_str());
        if (node.kind == baojiaozi::designer::BlueprintNodeKind::Control) {
            draw->AddCircleFilled({min.x, min.y + 45.0f}, 6.0f, IM_COL32(53, 207, 200, 255));
            draw->AddText({min.x + 12.0f, min.y + 37.0f}, IM_COL32(215, 220, 230, 255),
                          baojiaozi::document::ToString(node.controlType));
            draw->AddCircleFilled({max.x, min.y + 45.0f}, 6.0f, IM_COL32(53, 207, 200, 255));
        } else if (node.kind == baojiaozi::designer::BlueprintNodeKind::State) {
            draw->AddCircleFilled({min.x, min.y + 45.0f}, 6.0f, IM_COL32(186, 119, 233, 255));
            draw->AddText({min.x + 12.0f, min.y + 37.0f}, IM_COL32(215, 220, 230, 255), "状态");
            draw->AddCircleFilled({max.x, min.y + 45.0f}, 6.0f, IM_COL32(242, 174, 71, 255));
        } else {
            const auto value = node.value.dump();
            draw->AddCircleFilled({min.x, min.y + 45.0f}, 6.0f, IM_COL32(242, 174, 71, 255));
            draw->AddText({min.x + 12.0f, min.y + 39.0f}, IM_COL32(190, 200, 215, 255), value.c_str());
        }
    }
    draw->PopClipRect();
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
        const float workspaceHeight = ImGui::GetContentRegionAvail().y;
        const float previewHeight = std::max(240.0f, workspaceHeight * 0.48f);
        ImGui::BeginChild("preview_panel", ImVec2(0.0f, previewHeight), true);
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
        ImGui::EndChild();

        ImGui::BeginChild("blueprint_panel", ImVec2(0.0f, 0.0f), true,
                          ImGuiWindowFlags_AlwaysHorizontalScrollbar |
                          ImGuiWindowFlags_AlwaysVerticalScrollbar);
        ImGui::TextUnformatted("蓝图画布");
        ImGui::SameLine();
        ImGui::TextDisabled("控件 → 子控件 / 状态 → 属性");
        ImGui::Separator();
        if (editableProject) {
            auto pageIt = std::find_if(editableProject->pages.begin(), editableProject->pages.end(),
                                       [&](const auto& page) { return page.id == activePage; });
            if (pageIt != editableProject->pages.end()) {
                DrawBlueprintCanvas(baojiaozi::designer::BuildBlueprint(pageIt->root));
            }
        }
        ImGui::EndChild();
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

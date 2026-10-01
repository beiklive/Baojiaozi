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
#include <cmath>
#include <cstdio>
#include <functional>
#include <iostream>
#include <limits>
#include <optional>
#include <set>

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

struct BlueprintCanvasState {
    ImVec2 pan{24.0f, 24.0f};
    float zoom = 1.0f;
    std::string draggingNode;
    ImVec2 dragOffset{};
    bool panning = false;
    ImVec2 panAnchor{};
    ImVec2 panStart{};
    std::string pendingFrom;
    int selectedLink = -1;
};

constexpr float kBlueprintNodeWidth = 220.0f;

float BlueprintNodeHeight(const baojiaozi::designer::BlueprintNode& node) {
    return node.kind == baojiaozi::designer::BlueprintNodeKind::Property ? 64.0f : 58.0f;
}

baojiaozi::designer::BlueprintNode* FindBlueprintNode(
    baojiaozi::designer::BlueprintDocument& blueprint, const std::string& id) {
    const auto it = std::find_if(blueprint.nodes.begin(), blueprint.nodes.end(),
                                 [&](const auto& node) { return node.id == id; });
    return it == blueprint.nodes.end() ? nullptr : &*it;
}

ImVec2 BlueprintScreenPosition(const ImVec2& origin, const BlueprintCanvasState& state,
                               float x, float y) {
    return {origin.x + state.pan.x + x * state.zoom, origin.y + state.pan.y + y * state.zoom};
}

ImVec2 BlueprintGraphPosition(const ImVec2& origin, const BlueprintCanvasState& state,
                              const ImVec2& screen) {
    return {(screen.x - origin.x - state.pan.x) / state.zoom,
            (screen.y - origin.y - state.pan.y) / state.zoom};
}

bool IsValidBlueprintLink(const baojiaozi::designer::BlueprintDocument& blueprint,
                          const std::string& fromId, const std::string& toId,
                          baojiaozi::designer::BlueprintLinkKind& kind) {
    const auto from = std::find_if(blueprint.nodes.begin(), blueprint.nodes.end(),
                                   [&](const auto& node) { return node.id == fromId; });
    const auto to = std::find_if(blueprint.nodes.begin(), blueprint.nodes.end(),
                                 [&](const auto& node) { return node.id == toId; });
    if (from == blueprint.nodes.end() || to == blueprint.nodes.end()) return false;
    using NodeKind = baojiaozi::designer::BlueprintNodeKind;
    if (from->kind == NodeKind::Control && to->kind == NodeKind::Control) {
        kind = baojiaozi::designer::BlueprintLinkKind::Child;
        return true;
    }
    if (from->kind == NodeKind::Control && to->kind == NodeKind::State) {
        if (to->ownerId != from->id) return false;
        kind = baojiaozi::designer::BlueprintLinkKind::State;
        return true;
    }
    if (from->kind == NodeKind::State && to->kind == NodeKind::Property) {
        if (to->ownerId != from->ownerId || to->stateName != from->stateName) return false;
        kind = baojiaozi::designer::BlueprintLinkKind::Property;
        return true;
    }
    return false;
}

bool HasBlueprintLink(const baojiaozi::designer::BlueprintDocument& blueprint,
                      const std::string& from, const std::string& to) {
    return std::any_of(blueprint.links.begin(), blueprint.links.end(),
                       [&](const auto& link) { return link.from == from && link.to == to; });
}

void RemoveBlueprintNode(baojiaozi::designer::BlueprintDocument& blueprint, const std::string& rootId) {
    std::set<std::string> removed{rootId};
    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& link : blueprint.links) {
            if (removed.contains(link.from) && removed.insert(link.to).second) changed = true;
        }
    }
    blueprint.nodes.erase(std::remove_if(blueprint.nodes.begin(), blueprint.nodes.end(),
                                         [&](const auto& node) { return removed.contains(node.id); }),
                          blueprint.nodes.end());
    blueprint.links.erase(std::remove_if(blueprint.links.begin(), blueprint.links.end(),
                                         [&](const auto& link) {
                                             return removed.contains(link.from) || removed.contains(link.to);
                                         }),
                          blueprint.links.end());
}

float DistanceToBlueprintLink(const ImVec2& point, const ImVec2& a, const ImVec2& b) {
    const float bend = std::max(42.0f, std::abs(b.x - a.x) * 0.45f);
    const ImVec2 c1{a.x + bend, a.y};
    const ImVec2 c2{b.x - bend, b.y};
    float best = std::numeric_limits<float>::max();
    ImVec2 previous = a;
    for (int index = 1; index <= 20; ++index) {
        const float t = static_cast<float>(index) / 20.0f;
        const float u = 1.0f - t;
        const ImVec2 current{
            u * u * u * a.x + 3.0f * u * u * t * c1.x + 3.0f * u * t * t * c2.x + t * t * t * b.x,
            u * u * u * a.y + 3.0f * u * u * t * c1.y + 3.0f * u * t * t * c2.y + t * t * t * b.y};
        const ImVec2 segment{current.x - previous.x, current.y - previous.y};
        const float lengthSquared = segment.x * segment.x + segment.y * segment.y;
        const float projection = lengthSquared == 0.0f
                                     ? 0.0f
                                     : std::clamp(((point.x - previous.x) * segment.x +
                                                   (point.y - previous.y) * segment.y) / lengthSquared,
                                                  0.0f, 1.0f);
        const ImVec2 closest{previous.x + projection * segment.x, previous.y + projection * segment.y};
        best = std::min(best, std::hypot(point.x - closest.x, point.y - closest.y));
        previous = current;
    }
    return best;
}

void DrawBlueprintCanvas(baojiaozi::designer::BlueprintDocument& blueprint,
                         BlueprintCanvasState& state, std::string& selectedNode) {
    const ImVec2 available = ImGui::GetContentRegionAvail();
    const ImVec2 canvasSize(std::max(available.x, 900.0f), std::max(available.y, 520.0f));
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##blueprint_canvas", canvasSize,
                           ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle |
                           ImGuiButtonFlags_MouseButtonRight);
    const bool hovered = ImGui::IsItemHovered();
    const ImVec2 mouse = ImGui::GetIO().MousePos;
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 canvasMax(origin.x + canvasSize.x, origin.y + canvasSize.y);

    if (hovered && ImGui::GetIO().MouseWheel != 0.0f) {
        const ImVec2 graphBefore = BlueprintGraphPosition(origin, state, mouse);
        state.zoom = std::clamp(state.zoom + ImGui::GetIO().MouseWheel * 0.1f, 0.5f, 2.0f);
        const ImVec2 graphAfter = BlueprintScreenPosition(origin, state, graphBefore.x, graphBefore.y);
        state.pan.x += mouse.x - graphAfter.x;
        state.pan.y += mouse.y - graphAfter.y;
    }
    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Middle)) {
        state.panning = true;
        state.panAnchor = mouse;
        state.panStart = state.pan;
    }
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Middle)) state.panning = false;
    if (state.panning) {
        state.pan.x = state.panStart.x + mouse.x - state.panAnchor.x;
        state.pan.y = state.panStart.y + mouse.y - state.panAnchor.y;
    }

    auto findNode = [&](const std::string& id) { return FindBlueprintNode(blueprint, id); };
    auto nodeRect = [&](const baojiaozi::designer::BlueprintNode& node) {
        const ImVec2 min = BlueprintScreenPosition(origin, state, node.x, node.y);
        return std::pair<ImVec2, ImVec2>{min,
                                         {min.x + kBlueprintNodeWidth * state.zoom,
                                          min.y + BlueprintNodeHeight(node) * state.zoom}};
    };
    auto portPosition = [&](const baojiaozi::designer::BlueprintNode& node, bool output) {
        const auto [min, max] = nodeRect(node);
        return ImVec2{output ? max.x : min.x, min.y + (max.y - min.y) * 0.78f};
    };
    auto portAt = [&](const ImVec2& point, bool output) -> std::string {
        for (auto it = blueprint.nodes.rbegin(); it != blueprint.nodes.rend(); ++it) {
            if (it->kind == baojiaozi::designer::BlueprintNodeKind::Property && output) continue;
            if (it->kind == baojiaozi::designer::BlueprintNodeKind::Control ||
                it->kind == baojiaozi::designer::BlueprintNodeKind::State ||
                (!output && it->kind == baojiaozi::designer::BlueprintNodeKind::Property)) {
                const ImVec2 port = portPosition(*it, output);
                if (std::hypot(point.x - port.x, point.y - port.y) <= 10.0f) return it->id;
            }
        }
        return {};
    };
    auto nodeAt = [&](const ImVec2& point) -> std::string {
        for (auto it = blueprint.nodes.rbegin(); it != blueprint.nodes.rend(); ++it) {
            const auto [min, max] = nodeRect(*it);
            if (point.x >= min.x && point.x <= max.x && point.y >= min.y && point.y <= max.y) return it->id;
        }
        return {};
    };

    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        const std::string output = portAt(mouse, true);
        if (!output.empty()) {
            state.pendingFrom = output;
            selectedNode = output;
            state.selectedLink = -1;
        } else if (const std::string nodeId = nodeAt(mouse); !nodeId.empty()) {
            selectedNode = nodeId;
            state.selectedLink = -1;
            if (auto* node = findNode(nodeId)) {
                const ImVec2 graph = BlueprintGraphPosition(origin, state, mouse);
                state.draggingNode = nodeId;
                state.dragOffset = {graph.x - node->x, graph.y - node->y};
            }
        } else {
            state.selectedLink = -1;
            float bestDistance = 10.0f;
            for (std::size_t index = 0; index < blueprint.links.size(); ++index) {
                const auto& link = blueprint.links[index];
                auto* from = findNode(link.from);
                auto* to = findNode(link.to);
                if (from == nullptr || to == nullptr) continue;
                const ImVec2 a = portPosition(*from, true);
                const ImVec2 b = portPosition(*to, false);
                const float distance = DistanceToBlueprintLink(mouse, a, b);
                if (distance < bestDistance) {
                    bestDistance = distance;
                    state.selectedLink = static_cast<int>(index);
                }
            }
            selectedNode.clear();
        }
    }
    if (!state.draggingNode.empty() && ImGui::IsMouseDown(ImGuiMouseButton_Left) && state.pendingFrom.empty()) {
        if (auto* node = findNode(state.draggingNode)) {
            const ImVec2 graph = BlueprintGraphPosition(origin, state, mouse);
            node->x = graph.x - state.dragOffset.x;
            node->y = graph.y - state.dragOffset.y;
        }
    }
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        if (!state.pendingFrom.empty()) {
            const std::string target = portAt(mouse, false);
            baojiaozi::designer::BlueprintLinkKind kind;
            if (!target.empty() && target != state.pendingFrom &&
                IsValidBlueprintLink(blueprint, state.pendingFrom, target, kind) &&
                !HasBlueprintLink(blueprint, state.pendingFrom, target)) {
                blueprint.links.push_back({state.pendingFrom, target, kind});
            }
            state.pendingFrom.clear();
        }
        state.draggingNode.clear();
    }
    if (hovered && ImGui::IsKeyPressed(ImGuiKey_Delete)) {
        if (state.selectedLink >= 0 && state.selectedLink < static_cast<int>(blueprint.links.size())) {
            blueprint.links.erase(blueprint.links.begin() + state.selectedLink);
            state.selectedLink = -1;
        } else if (!selectedNode.empty()) {
            RemoveBlueprintNode(blueprint, selectedNode);
            selectedNode.clear();
        }
    }

    draw->AddRectFilled(origin, canvasMax, IM_COL32(17, 23, 31, 255));
    draw->PushClipRect(origin, canvasMax, true);
    const float grid = 24.0f * state.zoom;
    const float offsetX = std::fmod(state.pan.x, grid);
    const float offsetY = std::fmod(state.pan.y, grid);
    for (float x = offsetX; x < canvasSize.x; x += grid) {
        draw->AddLine({origin.x + x, origin.y}, {origin.x + x, canvasMax.y}, IM_COL32(25, 34, 45, 255));
    }
    for (float y = offsetY; y < canvasSize.y; y += grid) {
        draw->AddLine({origin.x, origin.y + y}, {canvasMax.x, origin.y + y}, IM_COL32(25, 34, 45, 255));
    }
    for (std::size_t index = 0; index < blueprint.links.size(); ++index) {
        const auto& link = blueprint.links[index];
        auto* from = findNode(link.from);
        auto* to = findNode(link.to);
        if (from == nullptr || to == nullptr) continue;
        const ImVec2 a = portPosition(*from, true);
        const ImVec2 b = portPosition(*to, false);
        const float bend = std::max(42.0f * state.zoom, std::abs(b.x - a.x) * 0.45f);
        const ImU32 color = state.selectedLink == static_cast<int>(index)
                                ? IM_COL32(255, 255, 255, 255)
                                : BlueprintLinkColor(link.kind);
        draw->AddBezierCubic(a, {a.x + bend, a.y}, {b.x - bend, b.y}, b, color,
                             state.selectedLink == static_cast<int>(index) ? 4.0f : 2.5f);
    }
    if (!state.pendingFrom.empty()) {
        if (auto* from = findNode(state.pendingFrom)) {
            const ImVec2 a = portPosition(*from, true);
            const float bend = std::max(42.0f * state.zoom, std::abs(mouse.x - a.x) * 0.45f);
            draw->AddBezierCubic(a, {a.x + bend, a.y}, {mouse.x - bend, mouse.y}, mouse,
                                 IM_COL32(240, 240, 240, 220), 2.0f);
        }
    }
    for (const auto& node : blueprint.nodes) {
        const auto [min, max] = nodeRect(node);
        const float headerHeight = 32.0f * state.zoom;
        const float portY = min.y + (max.y - min.y) * 0.78f;
        const bool selected = selectedNode == node.id;
        draw->AddRectFilled(min, max, IM_COL32(26, 33, 44, 255), 6.0f * state.zoom);
        draw->AddRectFilled(min, {max.x, min.y + headerHeight}, BlueprintNodeColor(node.kind), 6.0f * state.zoom);
        draw->AddRectFilled({min.x, min.y + headerHeight - 6.0f * state.zoom},
                            {max.x, min.y + headerHeight}, BlueprintNodeColor(node.kind));
        draw->AddRect(min, max, selected ? IM_COL32(255, 255, 255, 255) : IM_COL32(85, 100, 120, 255),
                      6.0f * state.zoom, 0, selected ? 2.0f : 1.0f);
        draw->AddText({min.x + 10.0f * state.zoom, min.y + 8.0f * state.zoom},
                      IM_COL32(245, 245, 245, 255), node.label.c_str());
        if (node.kind == baojiaozi::designer::BlueprintNodeKind::Control) {
            draw->AddCircleFilled({min.x, portY}, 6.0f * state.zoom, IM_COL32(53, 207, 200, 255));
            draw->AddText({min.x + 12.0f * state.zoom, min.y + 37.0f * state.zoom},
                          IM_COL32(215, 220, 230, 255), baojiaozi::document::ToString(node.controlType));
            draw->AddCircleFilled({max.x, portY}, 6.0f * state.zoom, IM_COL32(53, 207, 200, 255));
        } else if (node.kind == baojiaozi::designer::BlueprintNodeKind::State) {
            draw->AddCircleFilled({min.x, portY}, 6.0f * state.zoom, IM_COL32(186, 119, 233, 255));
            draw->AddText({min.x + 12.0f * state.zoom, min.y + 37.0f * state.zoom},
                          IM_COL32(215, 220, 230, 255), "状态");
            draw->AddCircleFilled({max.x, portY}, 6.0f * state.zoom, IM_COL32(242, 174, 71, 255));
        } else {
            const auto value = node.value.dump();
            draw->AddCircleFilled({min.x, portY}, 6.0f * state.zoom, IM_COL32(242, 174, 71, 255));
            draw->AddText({min.x + 12.0f * state.zoom, min.y + 39.0f * state.zoom},
                          IM_COL32(190, 200, 215, 255), value.c_str());
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
    std::optional<baojiaozi::designer::BlueprintDocument> editableBlueprint;
    std::string blueprintPage;
    BlueprintCanvasState blueprintCanvas;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (editableProject && blueprintPage != activePage) {
            const auto pageIt = std::find_if(editableProject->pages.begin(), editableProject->pages.end(),
                                             [&](const auto& page) { return page.id == activePage; });
            if (pageIt != editableProject->pages.end()) {
                const auto blueprintFile = ResourcePath("resources/examples/default_theme") /
                                            "pages" / (pageIt->id + ".blueprint.json");
                const auto loaded = baojiaozi::designer::BlueprintLoader().LoadFile(blueprintFile);
                if (loaded.Succeeded() && loaded.document->pageId == pageIt->id) {
                    editableBlueprint = *loaded.document;
                } else {
                    editableBlueprint = baojiaozi::designer::BuildBlueprint(*pageIt);
                }
                blueprintPage = activePage;
                blueprintCanvas = {};
                selectedNode.clear();
            }
        }

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("项目")) {
                if (ImGui::MenuItem("保存")) {
                    if (editableProject) {
                        std::string error;
                        bool saved = baojiaozi::designer::ProjectStore::SavePage(
                            *editableProject, activePage,
                            ResourcePath("resources/examples/default_theme"), error);
                        if (saved && editableBlueprint) {
                            saved = baojiaozi::designer::ProjectStore::SaveBlueprint(
                                *editableBlueprint, ResourcePath("resources/examples/default_theme"), error);
                        }
                        saveMessage = saved ? "已保存主页和蓝图" : error;
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
        if (editableBlueprint) DrawBlueprintCanvas(*editableBlueprint, blueprintCanvas, selectedNode);
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

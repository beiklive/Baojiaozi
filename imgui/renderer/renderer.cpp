#include "baojiaozi/imgui/renderer.hpp"

#include "imgui.h"

#include <algorithm>

namespace baojiaozi::imgui {
namespace {

ImU32 ToImColor(const style::Color& color) {
    return ImGui::ColorConvertFloat4ToU32(ImVec4(color.r, color.g, color.b, color.a));
}

void RenderNode(const runtime::RuntimeNode& node, const ImVec2 origin) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImVec2 min(origin.x + node.bounds.x, origin.y + node.bounds.y);
    const ImVec2 max(origin.x + node.bounds.x + node.bounds.width,
                     origin.y + node.bounds.y + node.bounds.height);
    const ImU32 background = ToImColor(node.style.background);
    if (node.type == document::NodeType::Box || node.type == document::NodeType::Button ||
        node.type == document::NodeType::Horizontal || node.type == document::NodeType::Vertical) {
        if (node.style.background.a > 0.0f) {
            drawList->AddRectFilled(min, max, background, node.style.cornerRadius);
        }
    }

    if (node.type == document::NodeType::Text || node.type == document::NodeType::Button) {
        const auto text = node.properties.value("text", std::string{});
        if (!text.empty()) {
            const ImU32 color = ToImColor(node.style.textColor);
            const ImVec2 textPosition(origin.x + node.bounds.x + node.style.padding + 8.0f,
                                      origin.y + node.bounds.y + node.style.padding + 8.0f);
            drawList->AddText(textPosition, color, text.c_str());
        }
    }

    if (node.type == document::NodeType::Image) {
        drawList->AddRect(min, max, ToImColor(node.style.textColor), node.style.cornerRadius, 0, 2.0f);
        const char* label = "IMAGE";
        const ImVec2 labelSize = ImGui::CalcTextSize(label);
        drawList->AddText({origin.x + node.bounds.x + (node.bounds.width - labelSize.x) / 2.0f,
                           origin.y + node.bounds.y + (node.bounds.height - labelSize.y) / 2.0f},
                          ToImColor(node.style.textColor), label);
    }

    for (const auto& child : node.children) RenderNode(child, origin);
}

} // namespace

void Renderer::Render(const runtime::RuntimeNode& node, const ImVec2 origin) const {
    RenderNode(node, origin);
}

} // namespace baojiaozi::imgui

#include "baojiaozi/runtime/runtime.hpp"

#include "baojiaozi/animation/player.hpp"

#include <algorithm>

namespace baojiaozi::runtime {
namespace {

float NumberProperty(const document::Json& properties, const char* key, float fallback) {
    const auto value = properties.value(key, document::Json());
    return value.is_number() ? value.get<float>() : fallback;
}

} // namespace

RuntimeNode Runtime::BuildNode(const document::Node& node,
                               const PreviewRequest& preview) const {
    RuntimeNode result;
    result.id = node.id;
    result.type = node.type;
    result.properties = node.properties;
    result.bindings = node.bindings;
    result.style = style::ThemeResolver(project_.theme).Resolve(node.type, node.style);
    const auto animationIt = node.animations.find(preview.event);
    if (animationIt != node.animations.end()) {
        const auto definitionIt = project_.animations.find(animationIt->second);
        if (definitionIt != project_.animations.end()) {
            const auto values = animation::Player::Evaluate(definitionIt->second, preview.timeSeconds);
            if (const auto value = values.find("fontSize"); value != values.end() && value->second.is_number()) {
                result.style.fontSize = value->second.get<float>();
            }
            if (const auto value = values.find("cornerRadius"); value != values.end() && value->second.is_number()) {
                result.style.cornerRadius = value->second.get<float>();
            }
            if (const auto value = values.find("padding"); value != values.end() && value->second.is_number()) {
                result.style.padding = value->second.get<float>();
            }
        }
    }
    result.children.reserve(node.children.size());
    for (const auto& child : node.children) result.children.push_back(BuildNode(child, preview));
    return result;
}

void Runtime::Layout(RuntimeNode& node, const Rect bounds) const {
    node.bounds = bounds;
    const float padding = NumberProperty(node.properties, "padding", node.style.padding);
    const float gap = NumberProperty(node.properties, "gap", node.style.gap);
    const float childWidth = std::max(0.0f, bounds.width - padding * 2.0f);
    const float defaultHeight = NumberProperty(node.properties, "height", node.type == document::NodeType::Text ? 32.0f : 64.0f);
    const float defaultWidth = NumberProperty(node.properties, "width", childWidth);

    if (node.type == document::NodeType::Vertical) {
        float y = bounds.y + padding;
        for (auto& child : node.children) {
            const float childDefaultHeight = child.type == document::NodeType::Text ? 32.0f : defaultHeight;
            const float height = NumberProperty(child.properties, "height", childDefaultHeight);
            Layout(child, {bounds.x + padding, y, childWidth, height});
            y += height + gap;
        }
    } else if (node.type == document::NodeType::Horizontal) {
        float x = bounds.x + padding;
        for (auto& child : node.children) {
            const float width = NumberProperty(child.properties, "width", defaultWidth / std::max<std::size_t>(1, node.children.size()));
            Layout(child, {x, bounds.y + padding, width, std::max(0.0f, bounds.height - padding * 2.0f)});
            x += width + gap;
        }
    } else {
        for (auto& child : node.children) {
            Layout(child, {bounds.x + padding, bounds.y + padding, childWidth, defaultHeight});
        }
    }
}

PageView Runtime::BuildPage(const std::string& pageId,
                            const Rect viewport,
                            const PreviewRequest preview) const {
    for (const auto& page : project_.pages) {
        if (page.id != pageId) continue;
        PageView view{page.id, page.title, BuildNode(page.root, preview)};
        Layout(view.root, viewport);
        return view;
    }
    return {};
}

} // namespace baojiaozi::runtime

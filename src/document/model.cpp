#include "baojiaozi/document/model.hpp"

namespace baojiaozi::document {

const char* ToString(const NodeType type) noexcept {
    switch (type) {
    case NodeType::Box: return "box";
    case NodeType::Text: return "text";
    case NodeType::Image: return "image";
    case NodeType::Button: return "button";
    case NodeType::Horizontal: return "horizontal";
    case NodeType::Vertical: return "vertical";
    case NodeType::Component: return "component";
    case NodeType::Unknown: return "unknown";
    }
    return "unknown";
}

NodeType NodeTypeFromString(const std::string& value) noexcept {
    if (value == "box") return NodeType::Box;
    if (value == "text") return NodeType::Text;
    if (value == "image") return NodeType::Image;
    if (value == "button") return NodeType::Button;
    if (value == "horizontal") return NodeType::Horizontal;
    if (value == "vertical") return NodeType::Vertical;
    if (value == "component") return NodeType::Component;
    return NodeType::Unknown;
}

Json SerializeNode(const Node& node) {
    Json result = {
        {"id", node.id},
        {"type", ToString(node.type)},
        {"properties", node.properties},
    };
    if (!node.style.empty()) result["style"] = node.style;
    if (!node.animations.empty()) result["animations"] = node.animations;
    if (!node.bindings.empty()) result["bindings"] = node.bindings;
    if (!node.children.empty()) {
        result["children"] = Json::array();
        for (const auto& child : node.children) result["children"].push_back(SerializeNode(child));
    }
    return result;
}

Json SerializeComponent(const ComponentDocument& component) {
    return {
        {"id", component.id},
        {"name", component.name},
        {"root", SerializeNode(component.root)},
    };
}

Json SerializePage(const PageDocument& page) {
    return {
        {"id", page.id},
        {"title", page.title},
        {"root", SerializeNode(page.root)},
    };
}

Json SerializeTheme(const ThemeDocument& theme) {
    return {
        {"tokens", theme.tokens},
        {"styles", theme.styles},
    };
}

Json SerializeManifest(const ManifestDocument& manifest) {
    return {
        {"schemaVersion", manifest.schemaVersion},
        {"application", manifest.application},
        {"pages", manifest.pages},
        {"actions", manifest.actions},
        {"states", manifest.states},
    };
}

} // namespace baojiaozi::document

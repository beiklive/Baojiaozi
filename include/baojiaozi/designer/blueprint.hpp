#pragma once

#include "baojiaozi/document/model.hpp"

#include <string>
#include <vector>

namespace baojiaozi::designer {

enum class BlueprintNodeKind {
    Control,
    State,
    Property,
};

enum class BlueprintLinkKind {
    Child,
    State,
    Property,
};

struct BlueprintNode {
    std::string id;
    BlueprintNodeKind kind = BlueprintNodeKind::Control;
    document::NodeType controlType = document::NodeType::Unknown;
    std::string label;
    std::string ownerId;
    std::string stateName;
    std::string propertyName;
    document::Json value;
    float x = 0.0f;
    float y = 0.0f;
};

struct BlueprintLink {
    std::string from;
    std::string to;
    BlueprintLinkKind kind = BlueprintLinkKind::Child;
};

struct BlueprintDocument {
    std::vector<BlueprintNode> nodes;
    std::vector<BlueprintLink> links;
};

[[nodiscard]] BlueprintDocument BuildBlueprint(const document::Node& root);

[[nodiscard]] constexpr const char* ToString(BlueprintNodeKind kind) noexcept {
    switch (kind) {
    case BlueprintNodeKind::Control: return "control";
    case BlueprintNodeKind::State: return "state";
    case BlueprintNodeKind::Property: return "property";
    }
    return "unknown";
}

} // namespace baojiaozi::designer

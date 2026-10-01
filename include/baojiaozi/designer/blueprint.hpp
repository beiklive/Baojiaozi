#pragma once

#include "baojiaozi/document/model.hpp"

#include <string>
#include <vector>

namespace baojiaozi::designer {

struct BlueprintNode {
    std::string id;
    document::NodeType type = document::NodeType::Unknown;
    std::string label;
    float x = 0.0f;
    float y = 0.0f;
};

struct BlueprintLink {
    std::string from;
    std::string to;
};

struct BlueprintDocument {
    std::vector<BlueprintNode> nodes;
    std::vector<BlueprintLink> links;
};

[[nodiscard]] BlueprintDocument BuildBlueprint(const document::Node& root);

} // namespace baojiaozi::designer

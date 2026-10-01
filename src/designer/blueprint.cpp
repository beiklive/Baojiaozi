#include "baojiaozi/designer/blueprint.hpp"

namespace baojiaozi::designer {
namespace {

void Append(const document::Node& node,
            const std::string& parent,
            float depth,
            float& row,
            BlueprintDocument& result) {
    result.nodes.push_back({node.id, node.type, node.id + " (" + document::ToString(node.type) + ")",
                            depth * 240.0f, row * 92.0f});
    if (!parent.empty()) result.links.push_back({parent, node.id});
    ++row;
    for (const auto& child : node.children) Append(child, node.id, depth + 1.0f, row, result);
}

} // namespace

BlueprintDocument BuildBlueprint(const document::Node& root) {
    BlueprintDocument result;
    float row = 0.0f;
    Append(root, {}, 0.0f, row, result);
    return result;
}

} // namespace baojiaozi::designer

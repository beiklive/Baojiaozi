#include "baojiaozi/designer/blueprint.hpp"

#include <algorithm>
#include <map>

namespace baojiaozi::designer {
namespace {

void Append(const document::Node& node,
            const std::string& parent,
            float depth,
            float& row,
            BlueprintDocument& result) {
    const float controlY = row;
    result.nodes.push_back({node.id, BlueprintNodeKind::Control, node.type,
                            node.id + " (" + document::ToString(node.type) + ")", {}, {}, {},
                            node.properties, depth * 250.0f, controlY});
    if (!parent.empty()) result.links.push_back({parent, node.id, BlueprintLinkKind::Child});

    std::map<std::string, document::Json> states;
    if (!node.properties.empty()) states.emplace("normal", node.properties);
    for (const auto& [stateName, properties] : node.states) {
        if (stateName == "normal") states[stateName] = properties;
        else states[stateName] = properties;
    }
    for (const auto& [eventName, animationId] : node.animations) {
        std::string stateName = eventName;
        if (eventName == "focus") stateName = "focused";
        else if (eventName == "trigger") stateName = "triggered";
        else if (eventName == "focus_out") stateName = "blurred";
        auto& properties = states[stateName];
        if (!properties.is_object()) properties = document::Json::object();
        properties["animation"] = animationId;
    }

    std::size_t stateIndex = 0;
    for (const auto& [stateName, properties] : states) {
        const std::string stateId = node.id + "::state::" + stateName;
        const float stateY = controlY + static_cast<float>(stateIndex) * 190.0f;
        result.nodes.push_back({stateId, BlueprintNodeKind::State, document::NodeType::Unknown,
                                stateName, node.id, stateName, {}, {},
                                depth * 250.0f + 250.0f, stateY});
        result.links.push_back({node.id, stateId, BlueprintLinkKind::State});
        std::size_t propertyIndex = 0;
        for (const auto& [propertyName, value] : properties.items()) {
            const std::string propertyId = stateId + "::property::" + propertyName;
            result.nodes.push_back({propertyId, BlueprintNodeKind::Property, document::NodeType::Unknown,
                                    propertyName, node.id, stateName, propertyName, value,
                                    depth * 250.0f + 500.0f,
                                    stateY + static_cast<float>(propertyIndex) * 58.0f});
            result.links.push_back({stateId, propertyId, BlueprintLinkKind::Property});
            ++propertyIndex;
        }
        ++stateIndex;
    }

    row += static_cast<float>(std::max<std::size_t>(1, states.size())) * 210.0f;
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

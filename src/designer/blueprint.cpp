#include "baojiaozi/designer/blueprint.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <map>
#include <set>
#include <tuple>
#include <unordered_map>

namespace baojiaozi::designer {
namespace {

using document::Json;

void AddDiagnostic(std::vector<BlueprintDiagnostic>& diagnostics,
                  const std::string& source,
                  const std::string& path,
                  const std::string& message) {
    diagnostics.push_back({source, path, message});
}

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
    for (const auto& [stateName, properties] : node.states) states[stateName] = properties;
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

bool ReadString(const Json& object,
                const char* key,
                std::string& target,
                std::vector<BlueprintDiagnostic>& diagnostics,
                const std::string& source,
                const std::string& path,
                bool required) {
    if (!object.contains(key)) {
        if (required) AddDiagnostic(diagnostics, source, path + "/" + key, "缺少字段");
        return !required;
    }
    if (!object.at(key).is_string()) {
        AddDiagnostic(diagnostics, source, path + "/" + key, "必须是字符串");
        return false;
    }
    target = object.at(key).get<std::string>();
    if (required && target.empty()) {
        AddDiagnostic(diagnostics, source, path + "/" + key, "不能为空");
        return false;
    }
    return true;
}

bool ReadPosition(const Json& object,
                  BlueprintNode& node,
                  std::vector<BlueprintDiagnostic>& diagnostics,
                  const std::string& source,
                  const std::string& path) {
    if (!object.contains("position") || !object.at("position").is_array() ||
        object.at("position").size() != 2 ||
        !object.at("position")[0].is_number() || !object.at("position")[1].is_number()) {
        AddDiagnostic(diagnostics, source, path + "/position", "必须是包含两个数字的数组");
        return false;
    }
    node.x = object.at("position")[0].get<float>();
    node.y = object.at("position")[1].get<float>();
    return true;
}

} // namespace

BlueprintNodeKind BlueprintNodeKindFromString(const std::string& value) noexcept {
    if (value == "control") return BlueprintNodeKind::Control;
    if (value == "state") return BlueprintNodeKind::State;
    if (value == "property") return BlueprintNodeKind::Property;
    return static_cast<BlueprintNodeKind>(-1);
}

BlueprintLinkKind BlueprintLinkKindFromString(const std::string& value) noexcept {
    if (value == "child") return BlueprintLinkKind::Child;
    if (value == "state") return BlueprintLinkKind::State;
    if (value == "property") return BlueprintLinkKind::Property;
    return static_cast<BlueprintLinkKind>(-1);
}

BlueprintDocument BuildBlueprint(const document::Node& root) {
    BlueprintDocument result;
    float row = 0.0f;
    Append(root, {}, 0.0f, row, result);
    return result;
}

BlueprintDocument BuildBlueprint(const document::PageDocument& page) {
    auto result = BuildBlueprint(page.root);
    result.pageId = page.id;
    return result;
}

std::vector<BlueprintDiagnostic> ValidateBlueprint(const BlueprintDocument& blueprint,
                                                   const std::string& source) {
    std::vector<BlueprintDiagnostic> diagnostics;
    if (blueprint.schemaVersion != 1) {
        AddDiagnostic(diagnostics, source, "/schemaVersion", "不支持的蓝图版本");
    }
    if (blueprint.pageId.empty()) {
        AddDiagnostic(diagnostics, source, "/pageId", "不能为空");
    }

    std::unordered_map<std::string, const BlueprintNode*> nodes;
    for (std::size_t index = 0; index < blueprint.nodes.size(); ++index) {
        const auto& node = blueprint.nodes[index];
        const auto path = "/nodes/" + std::to_string(index);
        if (node.id.empty()) AddDiagnostic(diagnostics, source, path + "/id", "不能为空");
        if (!node.id.empty() && !nodes.emplace(node.id, &node).second) {
            AddDiagnostic(diagnostics, source, path + "/id", "节点 ID 重复: " + node.id);
        }
        if (!std::isfinite(node.x) || !std::isfinite(node.y)) {
            AddDiagnostic(diagnostics, source, path + "/position", "位置必须是有限数值");
        }
        if (node.kind == BlueprintNodeKind::Control && node.controlType == document::NodeType::Unknown) {
            AddDiagnostic(diagnostics, source, path + "/controlType", "控件节点必须有有效控件类型");
        }
        if (node.kind == BlueprintNodeKind::State && (node.ownerId.empty() || node.stateName.empty())) {
            AddDiagnostic(diagnostics, source, path, "状态节点必须包含 ownerId 和 state");
        }
        if (node.kind == BlueprintNodeKind::Property && (node.ownerId.empty() || node.stateName.empty() ||
                                                          node.propertyName.empty())) {
            AddDiagnostic(diagnostics, source, path, "属性节点必须包含 ownerId、state 和 property");
        }
    }

    for (std::size_t index = 0; index < blueprint.nodes.size(); ++index) {
        const auto& node = blueprint.nodes[index];
        const auto path = "/nodes/" + std::to_string(index);
        if (node.kind == BlueprintNodeKind::State && !node.ownerId.empty()) {
            const auto owner = nodes.find(node.ownerId);
            if (owner == nodes.end() || owner->second->kind != BlueprintNodeKind::Control) {
                AddDiagnostic(diagnostics, source, path + "/ownerId", "状态 ownerId 必须引用控件节点");
            }
        }
        if (node.kind == BlueprintNodeKind::Property && !node.ownerId.empty()) {
            const auto owner = nodes.find(node.ownerId);
            if (owner == nodes.end() || owner->second->kind != BlueprintNodeKind::Control) {
                AddDiagnostic(diagnostics, source, path + "/ownerId", "属性 ownerId 必须引用控件节点");
            }
            const bool hasState = std::any_of(blueprint.nodes.begin(), blueprint.nodes.end(), [&](const auto& state) {
                return state.kind == BlueprintNodeKind::State && state.ownerId == node.ownerId &&
                       state.stateName == node.stateName;
            });
            if (!hasState) AddDiagnostic(diagnostics, source, path + "/state", "属性所属状态节点不存在");
        }
    }

    std::map<std::string, std::set<std::string>> children;
    std::set<std::tuple<std::string, std::string, BlueprintLinkKind>> links;
    for (std::size_t index = 0; index < blueprint.links.size(); ++index) {
        const auto& link = blueprint.links[index];
        const auto path = "/links/" + std::to_string(index);
        const auto from = nodes.find(link.from);
        const auto to = nodes.find(link.to);
        if (from == nodes.end() || to == nodes.end()) {
            AddDiagnostic(diagnostics, source, path, "连接端点必须引用已存在节点");
            continue;
        }
        if (!links.emplace(link.from, link.to, link.kind).second) {
            AddDiagnostic(diagnostics, source, path, "连接重复");
        }
        const bool validKinds =
            (link.kind == BlueprintLinkKind::Child && from->second->kind == BlueprintNodeKind::Control &&
             to->second->kind == BlueprintNodeKind::Control) ||
            (link.kind == BlueprintLinkKind::State && from->second->kind == BlueprintNodeKind::Control &&
             to->second->kind == BlueprintNodeKind::State) ||
            (link.kind == BlueprintLinkKind::Property && from->second->kind == BlueprintNodeKind::State &&
             to->second->kind == BlueprintNodeKind::Property);
        if (!validKinds) {
            AddDiagnostic(diagnostics, source, path, "连接类型与节点类型不匹配");
            continue;
        }
        if (link.kind == BlueprintLinkKind::Child) children[link.from].insert(link.to);
        if (link.kind == BlueprintLinkKind::State && to->second->ownerId != from->second->id) {
            AddDiagnostic(diagnostics, source, path, "状态节点 ownerId 与控件不一致");
        }
        if (link.kind == BlueprintLinkKind::Property &&
            (to->second->ownerId != from->second->ownerId || to->second->stateName != from->second->stateName)) {
            AddDiagnostic(diagnostics, source, path, "属性节点所属控件或状态不一致");
        }
    }

    std::set<std::string> visiting;
    std::set<std::string> visited;
    const auto visit = [&](const auto& self, const std::string& id) -> void {
        if (!visiting.insert(id).second) {
            AddDiagnostic(diagnostics, source, "/links", "控件父子连接存在循环");
            return;
        }
        for (const auto& child : children[id]) {
            if (!visited.contains(child)) self(self, child);
        }
        visiting.erase(id);
        visited.insert(id);
    };
    for (const auto& [id, node] : nodes) {
        if (node->kind == BlueprintNodeKind::Control && !visited.contains(id)) visit(visit, id);
    }
    return diagnostics;
}

BlueprintParseResult ParseBlueprint(const document::Json& value, const std::string& source) {
    BlueprintParseResult result;
    if (!value.is_object()) {
        AddDiagnostic(result.diagnostics, source, "/", "蓝图文件必须是对象");
        return result;
    }
    BlueprintDocument blueprint;
    if (!value.contains("schemaVersion") || !value.at("schemaVersion").is_number_integer()) {
        AddDiagnostic(result.diagnostics, source, "/schemaVersion", "必须是整数");
    } else {
        blueprint.schemaVersion = value.at("schemaVersion").get<int>();
    }
    ReadString(value, "pageId", blueprint.pageId, result.diagnostics, source, "/", true);
    if (!value.contains("nodes") || !value.at("nodes").is_array()) {
        AddDiagnostic(result.diagnostics, source, "/nodes", "必须是数组");
    } else {
        for (std::size_t index = 0; index < value.at("nodes").size(); ++index) {
            const auto& item = value.at("nodes")[index];
            const auto path = "/nodes/" + std::to_string(index);
            if (!item.is_object()) {
                AddDiagnostic(result.diagnostics, source, path, "节点必须是对象");
                continue;
            }
            BlueprintNode node;
            std::string kind;
            ReadString(item, "id", node.id, result.diagnostics, source, path, true);
            if (!ReadString(item, "kind", kind, result.diagnostics, source, path, true)) continue;
            node.kind = BlueprintNodeKindFromString(kind);
            if (node.kind == static_cast<BlueprintNodeKind>(-1)) {
                AddDiagnostic(result.diagnostics, source, path + "/kind", "未知节点类型: " + kind);
                continue;
            }
            ReadString(item, "label", node.label, result.diagnostics, source, path, false);
            ReadString(item, "ownerId", node.ownerId, result.diagnostics, source, path, false);
            ReadString(item, "state", node.stateName, result.diagnostics, source, path, false);
            ReadString(item, "property", node.propertyName, result.diagnostics, source, path, false);
            if (!ReadPosition(item, node, result.diagnostics, source, path)) continue;
            if (node.kind == BlueprintNodeKind::Control) {
                std::string controlType;
                if (!ReadString(item, "controlType", controlType, result.diagnostics, source, path, true)) continue;
                node.controlType = document::NodeTypeFromString(controlType);
                if (node.controlType == document::NodeType::Unknown) {
                    AddDiagnostic(result.diagnostics, source, path + "/controlType", "未知控件类型: " + controlType);
                }
            } else if (item.contains("value")) {
                node.value = item.at("value");
            }
            blueprint.nodes.push_back(std::move(node));
        }
    }
    if (!value.contains("links") || !value.at("links").is_array()) {
        AddDiagnostic(result.diagnostics, source, "/links", "必须是数组");
    } else {
        for (std::size_t index = 0; index < value.at("links").size(); ++index) {
            const auto& item = value.at("links")[index];
            const auto path = "/links/" + std::to_string(index);
            if (!item.is_object()) {
                AddDiagnostic(result.diagnostics, source, path, "连接必须是对象");
                continue;
            }
            BlueprintLink link;
            std::string kind;
            ReadString(item, "from", link.from, result.diagnostics, source, path, true);
            ReadString(item, "to", link.to, result.diagnostics, source, path, true);
            if (!ReadString(item, "kind", kind, result.diagnostics, source, path, true)) continue;
            link.kind = BlueprintLinkKindFromString(kind);
            if (link.kind == static_cast<BlueprintLinkKind>(-1)) {
                AddDiagnostic(result.diagnostics, source, path + "/kind", "未知连接类型: " + kind);
                continue;
            }
            blueprint.links.push_back(std::move(link));
        }
    }
    const auto validation = ValidateBlueprint(blueprint, source);
    result.diagnostics.insert(result.diagnostics.end(), validation.begin(), validation.end());
    if (result.diagnostics.empty()) result.document = std::move(blueprint);
    return result;
}

document::Json SerializeBlueprint(const BlueprintDocument& blueprint) {
    Json result = {
        {"schemaVersion", blueprint.schemaVersion},
        {"pageId", blueprint.pageId},
        {"nodes", Json::array()},
        {"links", Json::array()},
    };
    for (const auto& node : blueprint.nodes) {
        Json item = {
            {"id", node.id},
            {"kind", ToString(node.kind)},
            {"label", node.label},
            {"position", Json::array({node.x, node.y})},
        };
        if (node.kind == BlueprintNodeKind::Control) item["controlType"] = document::ToString(node.controlType);
        if (!node.ownerId.empty()) item["ownerId"] = node.ownerId;
        if (!node.stateName.empty()) item["state"] = node.stateName;
        if (!node.propertyName.empty()) item["property"] = node.propertyName;
        if (node.kind == BlueprintNodeKind::Property) item["value"] = node.value;
        result["nodes"].push_back(std::move(item));
    }
    for (const auto& link : blueprint.links) {
        result["links"].push_back({{"from", link.from}, {"to", link.to}, {"kind", ToString(link.kind)}});
    }
    return result;
}

BlueprintParseResult BlueprintLoader::LoadFile(const std::filesystem::path& file) const {
    std::ifstream stream(file);
    BlueprintParseResult result;
    if (!stream) {
        AddDiagnostic(result.diagnostics, file.string(), "/", "无法打开蓝图文件");
        return result;
    }
    try {
        Json value;
        stream >> value;
        return ParseBlueprint(value, file.string());
    } catch (const Json::exception& error) {
        AddDiagnostic(result.diagnostics, file.string(), "/", std::string("JSON 解析失败: ") + error.what());
        return result;
    }
}

} // namespace baojiaozi::designer

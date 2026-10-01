#pragma once

#include <nlohmann/json.hpp>

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace baojiaozi::document {

using Json = nlohmann::json;

enum class NodeType {
    Box,
    Text,
    Image,
    Button,
    Horizontal,
    Vertical,
    Component,
    Unknown,
};

[[nodiscard]] const char* ToString(NodeType type) noexcept;
[[nodiscard]] NodeType NodeTypeFromString(const std::string& value) noexcept;

struct Node {
    std::string id;
    NodeType type = NodeType::Unknown;
    Json properties = Json::object();
    std::string style;
    std::map<std::string, std::string> animations;
    std::map<std::string, Json> states;
    Json bindings = Json::object();
    std::vector<Node> children;
};

struct ComponentDocument {
    std::string id;
    std::string name;
    Node root;
};

struct PageDocument {
    std::string id;
    std::string title;
    Node root;
};

struct ThemeDocument {
    Json tokens = Json::object();
    Json styles = Json::object();
};

struct ManifestDocument {
    int schemaVersion = 1;
    std::string application;
    Json pages = Json::array();
    Json actions = Json::array();
    Json states = Json::array();
};

struct AnimationKeyframe {
    float time = 0.0f;
    Json value;
};

struct AnimationTrack {
    std::string property;
    std::string easing = "linear";
    std::vector<AnimationKeyframe> keyframes;
};

struct AnimationDocument {
    std::string id;
    float duration = 0.0f;
    std::vector<AnimationTrack> tracks;
};

struct ProjectDocument {
    int schemaVersion = 1;
    std::string projectId;
    std::string name;
    std::string targetFrontend;
    std::string targetCore;
    std::string manifestHash;
    ThemeDocument theme;
    ManifestDocument manifest;
    std::map<std::string, AnimationDocument> animations;
    std::vector<ComponentDocument> components;
    std::vector<PageDocument> pages;
};

[[nodiscard]] Json SerializeNode(const Node& node);
[[nodiscard]] Json SerializeComponent(const ComponentDocument& component);
[[nodiscard]] Json SerializePage(const PageDocument& page);
[[nodiscard]] Json SerializeTheme(const ThemeDocument& theme);
[[nodiscard]] Json SerializeManifest(const ManifestDocument& manifest);

} // namespace baojiaozi::document

#pragma once

#include "baojiaozi/document/model.hpp"

#include <filesystem>
#include <optional>
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

[[nodiscard]] constexpr const char* ToString(BlueprintLinkKind kind) noexcept {
    switch (kind) {
    case BlueprintLinkKind::Child: return "child";
    case BlueprintLinkKind::State: return "state";
    case BlueprintLinkKind::Property: return "property";
    }
    return "unknown";
}

[[nodiscard]] BlueprintNodeKind BlueprintNodeKindFromString(const std::string& value) noexcept;
[[nodiscard]] BlueprintLinkKind BlueprintLinkKindFromString(const std::string& value) noexcept;

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
    int schemaVersion = 1;
    std::string pageId;
    std::vector<BlueprintNode> nodes;
    std::vector<BlueprintLink> links;
};

[[nodiscard]] BlueprintDocument BuildBlueprint(const document::Node& root);
[[nodiscard]] BlueprintDocument BuildBlueprint(const document::PageDocument& page);

struct BlueprintDiagnostic {
    std::string source;
    std::string path;
    std::string message;
};

struct BlueprintParseResult {
    std::optional<BlueprintDocument> document;
    std::vector<BlueprintDiagnostic> diagnostics;

    [[nodiscard]] bool Succeeded() const noexcept { return document.has_value() && diagnostics.empty(); }
};

[[nodiscard]] BlueprintParseResult ParseBlueprint(const document::Json& value,
                                                  const std::string& source = {});
[[nodiscard]] document::Json SerializeBlueprint(const BlueprintDocument& blueprint);
[[nodiscard]] std::vector<BlueprintDiagnostic> ValidateBlueprint(const BlueprintDocument& blueprint,
                                                                  const std::string& source = {});

class BlueprintLoader {
public:
    [[nodiscard]] BlueprintParseResult LoadFile(const std::filesystem::path& file) const;
};

[[nodiscard]] constexpr const char* ToString(BlueprintNodeKind kind) noexcept {
    switch (kind) {
    case BlueprintNodeKind::Control: return "control";
    case BlueprintNodeKind::State: return "state";
    case BlueprintNodeKind::Property: return "property";
    }
    return "unknown";
}

} // namespace baojiaozi::designer

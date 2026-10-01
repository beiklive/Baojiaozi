#pragma once

#include "baojiaozi/document/model.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace baojiaozi::parser {

enum class DiagnosticSeverity {
    Warning,
    Error,
};

struct Diagnostic {
    DiagnosticSeverity severity = DiagnosticSeverity::Error;
    std::string source;
    std::string path;
    std::string message;
};

struct ProjectLoadResult {
    std::optional<document::ProjectDocument> project;
    std::vector<Diagnostic> diagnostics;

    [[nodiscard]] bool Succeeded() const noexcept;
};

class ProjectLoader {
public:
    [[nodiscard]] ProjectLoadResult LoadDirectory(
        const std::filesystem::path& directory) const;

    [[nodiscard]] ProjectLoadResult LoadJson(
        const document::Json& projectJson,
        const std::filesystem::path& baseDirectory,
        std::string sourceName = "project.json") const;
};

} // namespace baojiaozi::parser

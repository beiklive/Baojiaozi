#include "baojiaozi/parser/project_loader.hpp"

#include <algorithm>
#include <fstream>
#include <set>

namespace baojiaozi::parser {
namespace {

using document::Json;
using document::Node;

void Error(std::vector<Diagnostic>& diagnostics,
           const std::string& source,
           const std::string& path,
           const std::string& message) {
    diagnostics.push_back({DiagnosticSeverity::Error, source, path, message});
}

void Warning(std::vector<Diagnostic>& diagnostics,
             const std::string& source,
             const std::string& path,
             const std::string& message) {
    diagnostics.push_back({DiagnosticSeverity::Warning, source, path, message});
}

std::optional<Json> ReadJsonFile(const std::filesystem::path& file,
                                 std::vector<Diagnostic>& diagnostics) {
    std::ifstream stream(file);
    if (!stream) {
        Error(diagnostics, file.string(), "/", "无法打开 JSON 文件");
        return std::nullopt;
    }
    try {
        Json value;
        stream >> value;
        return value;
    } catch (const std::exception& error) {
        Error(diagnostics, file.string(), "/", std::string("JSON 解析失败: ") + error.what());
        return std::nullopt;
    }
}

std::optional<std::string> RequiredString(const Json& object,
                                          const char* key,
                                          const std::string& source,
                                          const std::string& path,
                                          std::vector<Diagnostic>& diagnostics) {
    if (!object.contains(key) || !object.at(key).is_string() || object.at(key).get<std::string>().empty()) {
        Error(diagnostics, source, path + "/" + key, "必须是非空字符串");
        return std::nullopt;
    }
    return object.at(key).get<std::string>();
}

std::optional<Node> ParseNode(const Json& value,
                              const std::string& source,
                              const std::string& path,
                              std::vector<Diagnostic>& diagnostics) {
    if (!value.is_object()) {
        Error(diagnostics, source, path, "节点必须是对象");
        return std::nullopt;
    }
    const auto id = RequiredString(value, "id", source, path, diagnostics);
    const auto type = RequiredString(value, "type", source, path, diagnostics);
    if (!id || !type) return std::nullopt;

    Node node;
    node.id = *id;
    node.type = document::NodeTypeFromString(*type);
    if (node.type == document::NodeType::Unknown) {
        Error(diagnostics, source, path + "/type", "未知控件类型: " + *type);
        return std::nullopt;
    }
    if (value.contains("properties")) {
        if (!value.at("properties").is_object()) {
            Error(diagnostics, source, path + "/properties", "必须是对象");
            return std::nullopt;
        }
        node.properties = value.at("properties");
    }
    if (value.contains("style")) {
        if (!value.at("style").is_string()) {
            Error(diagnostics, source, path + "/style", "必须是字符串");
            return std::nullopt;
        }
        node.style = value.at("style").get<std::string>();
    }
    if (value.contains("animations")) {
        if (!value.at("animations").is_object()) {
            Error(diagnostics, source, path + "/animations", "必须是对象");
            return std::nullopt;
        }
        for (const auto& [key, animation] : value.at("animations").items()) {
            if (!animation.is_string()) {
                Error(diagnostics, source, path + "/animations/" + key, "动画引用必须是字符串");
                return std::nullopt;
            }
            node.animations.emplace(key, animation.get<std::string>());
        }
    }
    if (value.contains("bindings")) {
        if (!value.at("bindings").is_object()) {
            Error(diagnostics, source, path + "/bindings", "必须是对象");
            return std::nullopt;
        }
        node.bindings = value.at("bindings");
    }
    if (value.contains("children")) {
        if (!value.at("children").is_array()) {
            Error(diagnostics, source, path + "/children", "必须是数组");
            return std::nullopt;
        }
        for (std::size_t index = 0; index < value.at("children").size(); ++index) {
            auto child = ParseNode(value.at("children").at(index), source,
                                   path + "/children/" + std::to_string(index), diagnostics);
            if (!child) return std::nullopt;
            node.children.push_back(std::move(*child));
        }
    }
    return node;
}

bool ValidateUniqueNodeIds(const Node& node, std::set<std::string>& ids,
                           const std::string& source, const std::string& path,
                           std::vector<Diagnostic>& diagnostics) {
    if (!ids.insert(node.id).second) {
        Error(diagnostics, source, path + "/id", "节点 ID 重复: " + node.id);
        return false;
    }
    bool valid = true;
    for (std::size_t index = 0; index < node.children.size(); ++index) {
        valid = ValidateUniqueNodeIds(node.children[index], ids, source,
                                      path + "/children/" + std::to_string(index), diagnostics) && valid;
    }
    return valid;
}

} // namespace

bool ProjectLoadResult::Succeeded() const noexcept {
    for (const auto& diagnostic : diagnostics) {
        if (diagnostic.severity == DiagnosticSeverity::Error) return false;
    }
    return project.has_value();
}

ProjectLoadResult ProjectLoader::LoadDirectory(const std::filesystem::path& directory) const {
    const auto projectFile = directory / "project.json";
    std::vector<Diagnostic> diagnostics;
    auto root = ReadJsonFile(projectFile, diagnostics);
    if (!root) return {std::nullopt, std::move(diagnostics)};
    return LoadJson(*root, directory, projectFile.string());
}

ProjectLoadResult ProjectLoader::LoadJson(const Json& projectJson,
                                          const std::filesystem::path& baseDirectory,
                                          std::string sourceName) const {
    std::vector<Diagnostic> diagnostics;
    if (!projectJson.is_object()) {
        Error(diagnostics, sourceName, "/", "项目文件必须是对象");
        return {std::nullopt, std::move(diagnostics)};
    }

    document::ProjectDocument project;
    if (projectJson.contains("schemaVersion") && projectJson.at("schemaVersion").is_number_integer()) {
        project.schemaVersion = projectJson.at("schemaVersion").get<int>();
    } else if (projectJson.contains("schemaVersion")) {
        Error(diagnostics, sourceName, "/schemaVersion", "必须是整数");
    }
    const auto projectId = RequiredString(projectJson, "projectId", sourceName, "", diagnostics);
    const auto name = RequiredString(projectJson, "name", sourceName, "", diagnostics);
    if (!projectId || !name) return {std::nullopt, std::move(diagnostics)};
    project.projectId = *projectId;
    project.name = *name;

    if (projectJson.contains("target") && projectJson.at("target").is_object()) {
        const auto& target = projectJson.at("target");
        if (target.contains("frontend") && target.at("frontend").is_string()) {
            project.targetFrontend = target.at("frontend").get<std::string>();
        }
        if (target.contains("core") && target.at("core").is_string()) {
            project.targetCore = target.at("core").get<std::string>();
        }
    } else {
        Warning(diagnostics, sourceName, "/target", "未指定 target，项目将使用通用目标");
    }
    if (projectJson.contains("manifestHash") && projectJson.at("manifestHash").is_string()) {
        project.manifestHash = projectJson.at("manifestHash").get<std::string>();
    }

    const auto themeFile = projectJson.value("theme", std::string("theme.json"));
    if (auto themeJson = ReadJsonFile(baseDirectory / themeFile, diagnostics)) {
        if (!themeJson->is_object()) {
            Error(diagnostics, (baseDirectory / themeFile).string(), "/", "主题文件必须是对象");
        } else {
            project.theme.tokens = themeJson->value("tokens", Json::object());
            project.theme.styles = themeJson->value("styles", Json::object());
        }
    }

    const auto manifestFile = projectJson.value("manifest", std::string("manifest.snapshot.json"));
    {
        if (auto manifestJson = ReadJsonFile(baseDirectory / manifestFile, diagnostics)) {
            if (!manifestJson->is_object()) {
                Error(diagnostics, (baseDirectory / manifestFile).string(), "/", "清单文件必须是对象");
            } else {
                project.manifest.schemaVersion = manifestJson->value("schemaVersion", 1);
                project.manifest.application = manifestJson->value("application", "");
                project.manifest.pages = manifestJson->value("pages", Json::array());
                project.manifest.actions = manifestJson->value("actions", Json::array());
                project.manifest.states = manifestJson->value("states", Json::array());
            }
        }
    }

    const auto componentsDirectory = baseDirectory / "components";
    if (std::filesystem::exists(componentsDirectory)) {
        std::vector<std::filesystem::path> files;
        for (const auto& entry : std::filesystem::directory_iterator(componentsDirectory)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") files.push_back(entry.path());
        }
        std::sort(files.begin(), files.end());
        for (const auto& file : files) {
            auto value = ReadJsonFile(file, diagnostics);
            if (!value || !value->is_object()) continue;
            const auto id = RequiredString(*value, "id", file.string(), "", diagnostics);
            if (!id) continue;
            const auto root = value->find("root");
            if (root == value->end()) {
                Error(diagnostics, file.string(), "/root", "缺少根节点");
                continue;
            }
            auto parsedRoot = ParseNode(*root, file.string(), "/root", diagnostics);
            if (!parsedRoot) continue;
            project.components.push_back({*id, value->value("name", *id), std::move(*parsedRoot)});
        }
    }

    const auto pagesDirectory = baseDirectory / "pages";
    if (std::filesystem::exists(pagesDirectory)) {
        std::vector<std::filesystem::path> files;
        for (const auto& entry : std::filesystem::directory_iterator(pagesDirectory)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") files.push_back(entry.path());
        }
        std::sort(files.begin(), files.end());
        for (const auto& file : files) {
            auto value = ReadJsonFile(file, diagnostics);
            if (!value || !value->is_object()) continue;
            const auto id = RequiredString(*value, "id", file.string(), "", diagnostics);
            if (!id) continue;
            const auto root = value->find("root");
            if (root == value->end()) {
                Error(diagnostics, file.string(), "/root", "缺少根节点");
                continue;
            }
            auto parsedRoot = ParseNode(*root, file.string(), "/root", diagnostics);
            if (!parsedRoot) continue;
            project.pages.push_back({*id, value->value("title", *id), std::move(*parsedRoot)});
        }
    }

    std::set<std::string> componentIds;
    for (const auto& component : project.components) {
        if (!componentIds.insert(component.id).second) {
            Error(diagnostics, "components", "/id", "组件 ID 重复: " + component.id);
        }
        std::set<std::string> nodeIds;
        ValidateUniqueNodeIds(component.root, nodeIds, "components/" + component.id, "/root", diagnostics);
    }
    std::set<std::string> pageIds;
    for (const auto& page : project.pages) {
        if (!pageIds.insert(page.id).second) {
            Error(diagnostics, "pages", "/id", "页面 ID 重复: " + page.id);
        }
        std::set<std::string> nodeIds;
        ValidateUniqueNodeIds(page.root, nodeIds, "pages/" + page.id, "/root", diagnostics);
    }

    return {std::move(project), std::move(diagnostics)};
}

} // namespace baojiaozi::parser

#include "baojiaozi/parser/project_loader.hpp"
#include "baojiaozi/version.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>

namespace {

void WriteFile(const std::filesystem::path& path, const char* contents) {
    std::ofstream stream(path);
    stream << contents;
}

} // namespace

int main() {
    assert(baojiaozi::Version() == "0.1.0-dev");

    const auto root = std::filesystem::temp_directory_path() / "baojiaozi_phase1_smoke";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root / "pages");
    std::filesystem::create_directories(root / "components");
    WriteFile(root / "project.json", R"({
        "schemaVersion": 1,
        "projectId": "smoke",
        "name": "Smoke Project",
        "target": {"frontend": "JiaoZiPi"}
    })");
    WriteFile(root / "theme.json", R"({
        "tokens": {"color.primary": "#4A90E2"},
        "styles": {}
    })");
    WriteFile(root / "manifest.snapshot.json", R"({
        "schemaVersion": 1,
        "application": "JiaoZiPi",
        "pages": [],
        "actions": [],
        "states": []
    })");
    WriteFile(root / "pages/home.json", R"({
        "id": "home",
        "title": "主页",
        "root": {
            "id": "root",
            "type": "vertical",
            "children": [
                {"id": "title", "type": "text", "properties": {"text": "JiaoZiPi"}},
                {"id": "open_library", "type": "button", "properties": {"text": "游戏库"}}
            ]
        }
    })");

    baojiaozi::parser::ProjectLoader loader;
    const auto result = loader.LoadDirectory(root);
    assert(result.Succeeded());
    assert(result.project->projectId == "smoke");
    assert(result.project->pages.size() == 1);
    assert(result.project->pages.front().root.children.size() == 2);

    WriteFile(root / "pages/broken.json", R"({"id":"broken","root":{"id":"bad","type":"not_a_control"}})");
    const auto broken = loader.LoadDirectory(root);
    assert(!broken.Succeeded());
    assert(!broken.diagnostics.empty());

    std::filesystem::remove_all(root);
    return 0;
}

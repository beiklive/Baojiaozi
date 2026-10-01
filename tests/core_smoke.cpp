#include "baojiaozi/parser/project_loader.hpp"
#include "baojiaozi/runtime/runtime.hpp"
#include "baojiaozi/animation/player.hpp"
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
    std::filesystem::create_directories(root / "animations");
    WriteFile(root / "animations/fade.json", R"({
        "id": "fade",
        "duration": 1.0,
        "tracks": [{
            "property": "opacity",
            "easing": "cubicOut",
            "keyframes": [{"time": 0.0, "value": 0.0}, {"time": 1.0, "value": 1.0}]
        }]
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
    assert(result.project->animations.contains("fade"));
    const auto values = baojiaozi::animation::Player::Evaluate(
        result.project->animations.at("fade"), 0.5f);
    assert(values.at("opacity").get<float>() > 0.5f);

    baojiaozi::runtime::Runtime runtime(*result.project);
    const auto view = runtime.BuildPage("home", {0.0f, 0.0f, 800.0f, 500.0f});
    assert(view.root.bounds.width == 800.0f);
    assert(view.root.children.front().bounds.width == 800.0f);
    assert(view.root.children.front().bounds.height == 32.0f);

    WriteFile(root / "pages/broken.json", R"({"id":"broken","root":{"id":"bad","type":"not_a_control"}})");
    const auto broken = loader.LoadDirectory(root);
    assert(!broken.Succeeded());
    assert(!broken.diagnostics.empty());

    std::filesystem::remove_all(root);
    return 0;
}

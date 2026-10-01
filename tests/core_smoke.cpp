#include "baojiaozi/parser/project_loader.hpp"
#include "baojiaozi/runtime/runtime.hpp"
#include "baojiaozi/animation/player.hpp"
#include "baojiaozi/designer/project_store.hpp"
#include "baojiaozi/designer/blueprint.hpp"
#include "baojiaozi/version.hpp"

#include <cassert>
#include <algorithm>
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
                {"id": "open_library", "type": "button", "properties": {"text": "游戏库"},
                 "states": {"focused": {"background": "#67A7F2", "cornerRadius": 10}}}
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
    const auto focusedView = runtime.BuildPage("home", {0.0f, 0.0f, 800.0f, 500.0f}, {"focus", 0.0f});
    assert(focusedView.root.children[1].style.background.g > 0.6f);
    std::string saveError;
    assert(baojiaozi::designer::ProjectStore::SavePage(*result.project, "home", root, saveError));
    assert(std::filesystem::is_regular_file(root / "pages/home.json"));
    const auto blueprint = baojiaozi::designer::BuildBlueprint(result.project->pages.front().root);
    const auto controlCount = std::count_if(blueprint.nodes.begin(), blueprint.nodes.end(), [](const auto& node) {
        return node.kind == baojiaozi::designer::BlueprintNodeKind::Control;
    });
    const auto stateLinkCount = std::count_if(blueprint.links.begin(), blueprint.links.end(), [](const auto& link) {
        return link.kind == baojiaozi::designer::BlueprintLinkKind::State;
    });
    const auto propertyLinkCount = std::count_if(blueprint.links.begin(), blueprint.links.end(), [](const auto& link) {
        return link.kind == baojiaozi::designer::BlueprintLinkKind::Property;
    });
    const auto stateCount = std::count_if(blueprint.nodes.begin(), blueprint.nodes.end(), [](const auto& node) {
        return node.kind == baojiaozi::designer::BlueprintNodeKind::State;
    });
    const auto propertyCount = std::count_if(blueprint.nodes.begin(), blueprint.nodes.end(), [](const auto& node) {
        return node.kind == baojiaozi::designer::BlueprintNodeKind::Property;
    });
    assert(controlCount == 3);
    assert(stateLinkCount == 3);
    assert(propertyLinkCount == 4);
    assert(stateCount == 3);
    assert(propertyCount == 4);

    const auto pageBlueprint = baojiaozi::designer::BuildBlueprint(result.project->pages.front());
    const auto blueprintJson = baojiaozi::designer::SerializeBlueprint(pageBlueprint);
    const auto parsedBlueprint = baojiaozi::designer::ParseBlueprint(blueprintJson, "roundtrip");
    assert(parsedBlueprint.Succeeded());
    assert(parsedBlueprint.document->pageId == "home");
    assert(parsedBlueprint.document->nodes.size() == pageBlueprint.nodes.size());
    assert(parsedBlueprint.document->links.size() == pageBlueprint.links.size());
    assert(baojiaozi::designer::ProjectStore::SaveBlueprint(pageBlueprint, root, saveError));
    assert(std::filesystem::is_regular_file(root / "pages/home.blueprint.json"));
    const auto loadedBlueprint = baojiaozi::designer::BlueprintLoader().LoadFile(
        root / "pages/home.blueprint.json");
    assert(loadedBlueprint.Succeeded());
    assert(loadedBlueprint.document->pageId == "home");

    auto invalidBlueprint = pageBlueprint;
    invalidBlueprint.nodes.push_back(invalidBlueprint.nodes.front());
    assert(!baojiaozi::designer::ValidateBlueprint(invalidBlueprint).empty());

    auto cyclicBlueprint = pageBlueprint;
    cyclicBlueprint.links.push_back({"title", "root", baojiaozi::designer::BlueprintLinkKind::Child});
    assert(!baojiaozi::designer::ValidateBlueprint(cyclicBlueprint).empty());

    WriteFile(root / "pages/broken.json", R"({"id":"broken","root":{"id":"bad","type":"not_a_control"}})");
    const auto broken = loader.LoadDirectory(root);
    assert(!broken.Succeeded());
    assert(!broken.diagnostics.empty());

    std::filesystem::remove_all(root);
    return 0;
}

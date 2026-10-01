#include "baojiaozi/designer/project_store.hpp"

#include <fstream>

namespace baojiaozi::designer {

bool ProjectStore::SavePage(const document::ProjectDocument& project,
                            const std::string& pageId,
                            const std::filesystem::path& directory,
                            std::string& error) {
    for (const auto& page : project.pages) {
        if (page.id != pageId) continue;
        const auto pagesDirectory = directory / "pages";
        std::error_code filesystemError;
        std::filesystem::create_directories(pagesDirectory, filesystemError);
        if (filesystemError) {
            error = filesystemError.message();
            return false;
        }
        const auto output = pagesDirectory / (page.id + ".json");
        std::ofstream stream(output);
        if (!stream) {
            error = "无法写入 " + output.string();
            return false;
        }
        stream << document::SerializePage(page).dump(2) << '\n';
        if (!stream.good()) {
            error = "写入失败: " + output.string();
            return false;
        }
        return true;
    }
    error = "找不到页面: " + pageId;
    return false;
}

bool ProjectStore::SaveBlueprint(const BlueprintDocument& blueprint,
                                 const std::filesystem::path& directory,
                                 std::string& error) {
    const auto diagnostics = ValidateBlueprint(blueprint);
    if (!diagnostics.empty()) {
        error = diagnostics.front().path + ": " + diagnostics.front().message;
        return false;
    }
    const auto pagesDirectory = directory / "pages";
    std::error_code filesystemError;
    std::filesystem::create_directories(pagesDirectory, filesystemError);
    if (filesystemError) {
        error = filesystemError.message();
        return false;
    }
    const auto output = pagesDirectory / (blueprint.pageId + ".blueprint.json");
    std::ofstream stream(output);
    if (!stream) {
        error = "无法写入 " + output.string();
        return false;
    }
    stream << SerializeBlueprint(blueprint).dump(2) << '\n';
    if (!stream.good()) {
        error = "写入失败: " + output.string();
        return false;
    }
    return true;
}

} // namespace baojiaozi::designer

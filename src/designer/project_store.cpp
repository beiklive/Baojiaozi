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

} // namespace baojiaozi::designer

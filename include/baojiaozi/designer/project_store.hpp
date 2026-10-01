#pragma once

#include "baojiaozi/document/model.hpp"
#include "baojiaozi/designer/blueprint.hpp"

#include <filesystem>
#include <string>

namespace baojiaozi::designer {

class ProjectStore {
public:
    [[nodiscard]] static bool SavePage(const document::ProjectDocument& project,
                                       const std::string& pageId,
                                       const std::filesystem::path& directory,
                                       std::string& error);
    [[nodiscard]] static bool SaveBlueprint(const BlueprintDocument& blueprint,
                                            const std::filesystem::path& directory,
                                            std::string& error);
};

} // namespace baojiaozi::designer

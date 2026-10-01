#pragma once

#include "baojiaozi/document/model.hpp"
#include "baojiaozi/style/style.hpp"

#include <string>
#include <vector>

namespace baojiaozi::runtime {

struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
};

struct RuntimeNode {
    std::string id;
    document::NodeType type = document::NodeType::Unknown;
    document::Json properties = document::Json::object();
    document::Json bindings = document::Json::object();
    style::VisualStyle style;
    Rect bounds;
    std::vector<RuntimeNode> children;
};

struct PageView {
    std::string id;
    std::string title;
    RuntimeNode root;
};

struct PreviewRequest {
    std::string event = "normal";
    float timeSeconds = 0.0f;
};

class Runtime {
public:
    explicit Runtime(const document::ProjectDocument& project) : project_(project) {}

    [[nodiscard]] PageView BuildPage(const std::string& pageId,
                                      Rect viewport = {0.0f, 0.0f, 1000.0f, 700.0f},
                                      PreviewRequest preview = {}) const;

private:
    [[nodiscard]] RuntimeNode BuildNode(const document::Node& node,
                                        const PreviewRequest& preview) const;
    void Layout(RuntimeNode& node, Rect bounds) const;

    const document::ProjectDocument& project_;
};

} // namespace baojiaozi::runtime

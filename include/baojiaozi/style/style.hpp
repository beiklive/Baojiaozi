#pragma once

#include "baojiaozi/document/model.hpp"

#include <string>

namespace baojiaozi::style {

struct Color {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;
};

struct VisualStyle {
    Color background{0.0f, 0.0f, 0.0f, 0.0f};
    Color textColor{1.0f, 1.0f, 1.0f, 1.0f};
    float fontSize = 18.0f;
    float cornerRadius = 0.0f;
    float padding = 0.0f;
    float gap = 0.0f;
};

class ThemeResolver {
public:
    explicit ThemeResolver(const document::ThemeDocument& theme) : theme_(theme) {}

    [[nodiscard]] VisualStyle Resolve(document::NodeType type,
                                      const std::string& styleId) const;

private:
    [[nodiscard]] Color ResolveColor(const document::Json& value,
                                     const Color& fallback) const;
    [[nodiscard]] document::Json ResolveToken(const document::Json& value) const;

    const document::ThemeDocument& theme_;
};

} // namespace baojiaozi::style

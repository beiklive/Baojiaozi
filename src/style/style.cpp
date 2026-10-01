#include "baojiaozi/style/style.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>

namespace baojiaozi::style {
namespace {

float ParseChannel(const std::string& value, std::size_t offset) {
    unsigned int channel = 0;
    const auto result = std::from_chars(value.data() + offset, value.data() + offset + 2, channel, 16);
    return result.ec == std::errc{} ? static_cast<float>(channel) / 255.0f : 1.0f;
}

} // namespace

document::Json ThemeResolver::ResolveToken(const document::Json& value) const {
    if (value.is_string()) {
        const auto text = value.get<std::string>();
        if (!text.empty() && text.front() == '$') {
            const auto key = text.substr(1);
            if (theme_.tokens.contains(key)) return theme_.tokens.at(key);
        }
    }
    return value;
}

Color ThemeResolver::ResolveColor(const document::Json& value, const Color& fallback) const {
    const auto resolved = ResolveToken(value);
    if (!resolved.is_string()) return fallback;
    const auto text = resolved.get<std::string>();
    if (text.size() != 7 || text.front() != '#') return fallback;
    return {ParseChannel(text, 1), ParseChannel(text, 3), ParseChannel(text, 5), 1.0f};
}

VisualStyle ThemeResolver::Resolve(const document::NodeType type,
                                   const std::string& styleId) const {
    VisualStyle result;
    const auto typeStyle = theme_.styles.value(document::ToString(type), document::Json::object());
    const auto namedStyle = styleId.empty()
        ? document::Json::object()
        : theme_.styles.value(styleId, document::Json::object());

    auto read = [&](const char* key) -> document::Json {
        if (namedStyle.is_object() && namedStyle.contains(key)) return namedStyle.at(key);
        if (typeStyle.is_object() && typeStyle.contains(key)) return typeStyle.at(key);
        return document::Json();
    };
    if (const auto value = read("background"); !value.is_null()) result.background = ResolveColor(value, result.background);
    if (const auto value = read("textColor"); !value.is_null()) result.textColor = ResolveColor(value, result.textColor);
    if (const auto value = read("fontSize"); value.is_number()) result.fontSize = value.get<float>();
    if (const auto value = read("cornerRadius"); value.is_number()) result.cornerRadius = value.get<float>();
    if (const auto value = read("padding"); value.is_number()) result.padding = value.get<float>();
    if (const auto value = read("gap"); value.is_number()) result.gap = value.get<float>();
    return result;
}

VisualStyle ThemeResolver::ApplyOverrides(VisualStyle result,
                                          const document::Json& properties) const {
    if (!properties.is_object()) return result;
    if (properties.contains("background")) result.background = ResolveColor(properties.at("background"), result.background);
    if (properties.contains("textColor")) result.textColor = ResolveColor(properties.at("textColor"), result.textColor);
    if (properties.contains("fontSize") && properties.at("fontSize").is_number()) result.fontSize = properties.at("fontSize").get<float>();
    if (properties.contains("cornerRadius") && properties.at("cornerRadius").is_number()) result.cornerRadius = properties.at("cornerRadius").get<float>();
    if (properties.contains("padding") && properties.at("padding").is_number()) result.padding = properties.at("padding").get<float>();
    if (properties.contains("gap") && properties.at("gap").is_number()) result.gap = properties.at("gap").get<float>();
    return result;
}

} // namespace baojiaozi::style

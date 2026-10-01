#pragma once

#include <string_view>

namespace baojiaozi {

[[nodiscard]] constexpr std::string_view Version() noexcept {
    return "0.1.0-dev";
}

} // namespace baojiaozi

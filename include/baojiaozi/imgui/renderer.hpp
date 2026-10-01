#pragma once

#include "baojiaozi/runtime/runtime.hpp"

#include <imgui.h>

namespace baojiaozi::imgui {

class Renderer {
public:
    void Render(const runtime::RuntimeNode& node, ImVec2 origin = {}) const;
};

} // namespace baojiaozi::imgui

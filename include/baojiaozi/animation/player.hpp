#pragma once

#include "baojiaozi/document/model.hpp"

#include <map>
#include <string>

namespace baojiaozi::animation {

using EvaluatedProperties = std::map<std::string, document::Json>;

class Player {
public:
    [[nodiscard]] static EvaluatedProperties Evaluate(
        const document::AnimationDocument& animation,
        float timeSeconds);
};

} // namespace baojiaozi::animation

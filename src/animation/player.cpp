#include "baojiaozi/animation/player.hpp"

#include <algorithm>
#include <cmath>

namespace baojiaozi::animation {
namespace {

float Ease(const std::string& name, float value) {
    value = std::clamp(value, 0.0f, 1.0f);
    if (name == "step") return value < 1.0f ? 0.0f : 1.0f;
    if (name == "cubicIn") return value * value * value;
    if (name == "cubicOut") {
        const float inverse = 1.0f - value;
        return 1.0f - inverse * inverse * inverse;
    }
    if (name == "cubicInOut") {
        return value < 0.5f
            ? 4.0f * value * value * value
            : 1.0f - std::pow(-2.0f * value + 2.0f, 3.0f) / 2.0f;
    }
    return value;
}

document::Json Interpolate(const document::Json& from,
                           const document::Json& to,
                           float t) {
    if (from.is_number() && to.is_number()) {
        return from.get<double>() + (to.get<double>() - from.get<double>()) * t;
    }
    if (from.is_array() && to.is_array() && from.size() == to.size()) {
        document::Json result = document::Json::array();
        for (std::size_t index = 0; index < from.size(); ++index) {
            result.push_back(Interpolate(from.at(index), to.at(index), t));
        }
        return result;
    }
    return t < 1.0f ? from : to;
}

} // namespace

EvaluatedProperties Player::Evaluate(const document::AnimationDocument& animation,
                                     const float timeSeconds) {
    EvaluatedProperties result;
    const float time = std::clamp(timeSeconds, 0.0f, std::max(0.0f, animation.duration));
    for (const auto& track : animation.tracks) {
        if (track.keyframes.empty()) continue;
        if (track.keyframes.size() == 1 || time <= track.keyframes.front().time) {
            result[track.property] = track.keyframes.front().value;
            continue;
        }
        if (time >= track.keyframes.back().time) {
            result[track.property] = track.keyframes.back().value;
            continue;
        }
        for (std::size_t index = 1; index < track.keyframes.size(); ++index) {
            const auto& previous = track.keyframes[index - 1];
            const auto& next = track.keyframes[index];
            if (time > next.time) continue;
            const float span = std::max(0.000001f, next.time - previous.time);
            const float normalized = Ease(track.easing, (time - previous.time) / span);
            result[track.property] = Interpolate(previous.value, next.value, normalized);
            break;
        }
    }
    return result;
}

} // namespace baojiaozi::animation

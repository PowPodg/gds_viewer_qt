#include "../include/GdsElements.hpp"

#include <stdexcept>
#include <utility>
#include <variant>

namespace gds {

namespace {

template <typename T>
const T& getElement(const GdsElement& element) {
    return std::get<T>(element);
}

} // namespace

const char* elementName(const GdsElement& element) {
    return std::visit([](const auto& value) -> const char* {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, Boundary>) return "BOUNDARY";
        if constexpr (std::is_same_v<T, Path>) return "PATH";
        if constexpr (std::is_same_v<T, SRef>) return "SREF";
        if constexpr (std::is_same_v<T, ARef>) return "AREF";
        if constexpr (std::is_same_v<T, Text>) return "TEXT";
        if constexpr (std::is_same_v<T, Node>) return "NODE";
        if constexpr (std::is_same_v<T, Box>) return "BOX";
        return "UNKNOWN";
    }, element);
}

void validateElement(const GdsElement& element) {
    std::visit([](const auto& value) {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, Boundary>) {
            if (value.xy.size() < 4) {
                throw GdsError("BOUNDARY must contain at least 4 coordinates");
            }
            if (value.xy.front().x != value.xy.back().x || value.xy.front().y != value.xy.back().y) {
                throw GdsError("BOUNDARY first and last coordinates must coincide");
            }
        } else if constexpr (std::is_same_v<T, Path>) {
            if (value.xy.size() < 2) {
                throw GdsError("PATH must contain at least 2 coordinates");
            }
        } else if constexpr (std::is_same_v<T, SRef>) {
            (void)value;
        } else if constexpr (std::is_same_v<T, ARef>) {
            (void)value;
        } else if constexpr (std::is_same_v<T, Text>) {
            (void)value;
        } else if constexpr (std::is_same_v<T, Node>) {
            if (value.xy.empty() || value.xy.size() > 50) {
                throw GdsError("NODE must contain between 1 and 50 coordinates");
            }
        } else if constexpr (std::is_same_v<T, Box>) {
            if (value.xy.size() != 5) {
                throw GdsError("BOX must contain exactly 5 coordinates");
            }
            if (value.xy.front().x != value.xy.back().x || value.xy.front().y != value.xy.back().y) {
                throw GdsError("BOX first and last coordinates must coincide");
            }
        }
    }, element);
}

} // namespace gds

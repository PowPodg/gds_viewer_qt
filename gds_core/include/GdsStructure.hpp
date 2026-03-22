#pragma once

#include "GdsElements.hpp"
#include "GdsTypes.hpp"

#include <optional>
#include <string>
#include <vector>

namespace gds {

struct GdsStructure {
    Timestamp created{};
    Timestamp modified{};
    std::string name;
    std::optional<std::uint16_t> strclass;
    std::vector<GdsElement> elements;

    void addElement(GdsElement element);
};

} // namespace gds

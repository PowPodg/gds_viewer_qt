#pragma once

#include "GdsStructure.hpp"
#include "GdsTypes.hpp"

#include <optional>
#include <string>
#include <vector>

namespace gds {

struct GdsLibrary {
    std::int16_t headerVersion{};
    Timestamp lastModified{};
    Timestamp lastAccessed{};
    std::string name;
    std::vector<std::string> reflibs;
    std::vector<std::string> fonts;
    std::optional<std::string> attrtable;
    std::optional<std::int16_t> generations;
    std::optional<std::int16_t> format;
    std::vector<std::string> masks;
    Units units{};
    std::vector<GdsStructure> structures;

    void addStructure(GdsStructure structure);
    const GdsStructure* findStructure(const std::string& structureName) const;
};

} // namespace gds

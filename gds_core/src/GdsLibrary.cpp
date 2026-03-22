#include "../include/GdsLibrary.hpp"

namespace gds {

void GdsLibrary::addStructure(GdsStructure structure) {
    structures.push_back(std::move(structure));
}

const GdsStructure* GdsLibrary::findStructure(const std::string& structureName) const {
    for (const auto& structure : structures) {
        if (structure.name == structureName) {
            return &structure;
        }
    }
    return nullptr;
}

} // namespace gds

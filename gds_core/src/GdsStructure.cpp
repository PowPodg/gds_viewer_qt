#include "../include/GdsStructure.hpp"

namespace gds {

void GdsStructure::addElement(GdsElement element) {
    validateElement(element);
    elements.push_back(std::move(element));
}

} // namespace gds

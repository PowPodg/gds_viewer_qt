#pragma once

#include "GdsLibrary.hpp"

#include <istream>
#include <string>

namespace gds {

class GdsReader {
public:
    GdsLibrary readFile(const std::string& path) const;
    GdsLibrary readStream(std::istream& input) const;
};

} // namespace gds

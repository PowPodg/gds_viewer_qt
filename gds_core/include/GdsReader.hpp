#pragma once

#include "GdsLibrary.hpp"
#include "GdsProgress.hpp"

#include <istream>
#include <string>

namespace gds {

class GdsReader {
public:
    GdsLibrary readFile(const std::string& path) const;
    GdsLibrary readStream(std::istream& input) const;

    GdsLibrary readFileWithProgress(const std::string& path,
        GdsReadProgressCallback onProgress,
        GdsReadProgressOptions progressOptions = {}) const;

    GdsLibrary readStreamWithProgress(std::istream& input,
        std::optional<std::size_t> totalBytes,
        GdsReadProgressCallback onProgress,
        GdsReadProgressOptions progressOptions = {}) const;
};

} // namespace gds

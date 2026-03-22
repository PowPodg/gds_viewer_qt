#include "../include/GdsReader.hpp"

#include "internal/InputStream.hpp"
#include "internal/Parser.hpp"
#include "internal/RecordReader.hpp"

#include <fstream>

namespace gds {

GdsLibrary GdsReader::readFile(const std::string& path) const {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw GdsError("Failed to open GDS file: " + path);
    }
    return readStream(input);
}

GdsLibrary GdsReader::readStream(std::istream& input) const {
    internal::InputStream stream(input);
    internal::RecordReader recordReader(stream);
    internal::Parser parser(recordReader);
    return parser.parse();
}

} // namespace gds

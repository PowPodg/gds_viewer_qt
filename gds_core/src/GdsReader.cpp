#include "../include/GdsReader.hpp"

#include "internal/InputStream.hpp"
#include "internal/Parser.hpp"
#include "internal/RecordReader.hpp"

#include <filesystem>
#include <fstream>
#include <system_error>
#include <utility>

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

GdsLibrary GdsReader::readFileWithProgress(const std::string& path,
    GdsReadProgressCallback onProgress,
    GdsReadProgressOptions progressOptions) const {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw GdsError("Failed to open GDS file: " + path);
    }

    std::optional<std::size_t> totalBytes;
    {
        std::error_code ec;
        const auto size = std::filesystem::file_size(path, ec);
        if (!ec) {
            totalBytes = static_cast<std::size_t>(size);
        }
    }

    return readStreamWithProgress(input, totalBytes, std::move(onProgress), std::move(progressOptions));
}

GdsLibrary GdsReader::readStreamWithProgress(std::istream& input,
    std::optional<std::size_t> totalBytes,
    GdsReadProgressCallback onProgress,
    GdsReadProgressOptions progressOptions) const {
    internal::InputStream stream(input);
    internal::RecordReader recordReader(stream);
    internal::Parser parser(recordReader, totalBytes, std::move(onProgress), std::move(progressOptions));
    return parser.parse();
}

} // namespace gds

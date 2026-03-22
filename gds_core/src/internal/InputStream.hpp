#pragma once

#include "../../include/GdsTypes.hpp"

#include <cstddef>
#include <cstdint>
#include <istream>
#include <vector>

namespace gds::internal {

class InputStream {
public:
    explicit InputStream(std::istream& input);

    std::size_t position() const noexcept;
    bool eof() const;
    bool consumeTrailingZeroPadding();

    std::uint8_t readU8();
    std::uint16_t readU16BE();
    std::int16_t readI16BE();
    std::int32_t readI32BE();
    std::vector<std::uint8_t> readBytes(std::size_t count);

private:
    std::istream* input_{};
    std::size_t position_{};
};

} // namespace gds::internal

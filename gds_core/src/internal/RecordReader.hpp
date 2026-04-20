#pragma once

#include "InputStream.hpp"
#include "Record.hpp"
#include <cstddef>

namespace gds::internal {

class RecordReader {
public:
    explicit RecordReader(InputStream& input);

    bool eof();
    bool consumeTrailingZeroPadding();

    std::size_t position() const noexcept;
    std::size_t recordsRead() const noexcept;

    Record read();

private:
    InputStream* input_{};
    std::size_t recordsRead_{ 0 };
};

} // namespace gds::internal

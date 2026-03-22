#pragma once

#include "InputStream.hpp"
#include "Record.hpp"

#include <optional>

namespace gds::internal {

class RecordReader {
public:
    explicit RecordReader(InputStream& input);

    bool eof();
    bool consumeTrailingZeroPadding();
    Record read();

private:
    InputStream* input_{};
};

} // namespace gds::internal

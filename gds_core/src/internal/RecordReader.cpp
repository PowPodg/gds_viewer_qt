#include "RecordReader.hpp"

namespace gds::internal {

RecordReader::RecordReader(InputStream& input) : input_(&input) {}

bool RecordReader::eof() {
    return input_->eof();
}

bool RecordReader::consumeTrailingZeroPadding() {
    return input_->consumeTrailingZeroPadding();
}

Record RecordReader::read() {
    const std::uint16_t length = input_->readU16BE();
    if (length < 4) {
        throw GdsError("Invalid GDS record length: must be >= 4");
    }
    if ((length % 2U) != 0U) {
        throw GdsError("Invalid GDS record length: must be even");
    }

    const auto recordType = static_cast<RecordType>(input_->readU8());
    const auto dataType = static_cast<DataType>(input_->readU8());
    const std::size_t payloadSize = static_cast<std::size_t>(length - 4U);
    auto payload = input_->readBytes(payloadSize);

    return Record{RecordHeader{length, recordType, dataType}, std::move(payload)};
}

} // namespace gds::internal

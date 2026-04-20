#include "RecordReader.hpp"

namespace gds::internal {

RecordReader::RecordReader(InputStream& input) : input_(&input) {}

bool RecordReader::eof() {
    return input_->eof();
}

bool RecordReader::consumeTrailingZeroPadding() {
    return input_->consumeTrailingZeroPadding();
}

std::size_t RecordReader::position() const noexcept {
    return input_->position();
}

std::size_t RecordReader::recordsRead() const noexcept {
    return recordsRead_;
}

// Reads one raw GDS record:
// 2 bytes length, 1 byte record type, 1 byte data type, then payload.
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

    ++recordsRead_;

    return Record{RecordHeader{length, recordType, dataType}, std::move(payload)};
}

} // namespace gds::internal

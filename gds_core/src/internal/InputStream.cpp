#include "InputStream.hpp"

#include <array>

namespace gds::internal {

InputStream::InputStream(std::istream& input) : input_(&input) {}

std::size_t InputStream::position() const noexcept {
    return position_;
}

bool InputStream::eof() const {
    return input_->peek() == std::char_traits<char>::eof();
}

bool InputStream::consumeTrailingZeroPadding() {
    while (true) {
        const int ch = input_->peek();
        if (ch == std::char_traits<char>::eof()) {
            return true;
        }
        if (ch != 0) {
            return false;
        }
        input_->get();
        ++position_;
    }
}
std::uint8_t InputStream::readU8() {
    const int ch = input_->get();
    if (ch == std::char_traits<char>::eof()) {
        throw GdsError("Unexpected EOF while reading byte");
    }
    ++position_;
    return static_cast<std::uint8_t>(ch);
}

std::uint16_t InputStream::readU16BE() {
    const std::uint16_t b0 = readU8();
    const std::uint16_t b1 = readU8();
    return static_cast<std::uint16_t>((b0 << 8U) | b1);
}

std::int16_t InputStream::readI16BE() {
    return static_cast<std::int16_t>(readU16BE());
}

std::int32_t InputStream::readI32BE() {
    const std::uint32_t b0 = readU8();
    const std::uint32_t b1 = readU8();
    const std::uint32_t b2 = readU8();
    const std::uint32_t b3 = readU8();
    const std::uint32_t value = (b0 << 24U) | (b1 << 16U) | (b2 << 8U) | b3;
    return static_cast<std::int32_t>(value);
}

std::vector<std::uint8_t> InputStream::readBytes(std::size_t count) {
    std::vector<std::uint8_t> result(count);
    if (!input_->read(reinterpret_cast<char*>(result.data()), static_cast<std::streamsize>(count))) {
        throw GdsError("Unexpected EOF while reading byte array");
    }
    position_ += count;
    return result;
}

} // namespace gds::internal

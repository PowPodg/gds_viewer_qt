#include "Record.hpp"

#include <cmath>

namespace gds::internal {

const char* toString(RecordType type) {
    switch (type) {
    case RecordType::HEADER: return "HEADER";
    case RecordType::BGNLIB: return "BGNLIB";
    case RecordType::LIBNAME: return "LIBNAME";
    case RecordType::UNITS: return "UNITS";
    case RecordType::ENDLIB: return "ENDLIB";
    case RecordType::BGNSTR: return "BGNSTR";
    case RecordType::STRNAME: return "STRNAME";
    case RecordType::ENDSTR: return "ENDSTR";
    case RecordType::BOUNDARY: return "BOUNDARY";
    case RecordType::PATH: return "PATH";
    case RecordType::SREF: return "SREF";
    case RecordType::AREF: return "AREF";
    case RecordType::TEXT: return "TEXT";
    case RecordType::LAYER: return "LAYER";
    case RecordType::DATATYPE: return "DATATYPE";
    case RecordType::WIDTH: return "WIDTH";
    case RecordType::XY: return "XY";
    case RecordType::ENDEL: return "ENDEL";
    case RecordType::SNAME: return "SNAME";
    case RecordType::COLROW: return "COLROW";
    case RecordType::NODE: return "NODE";
    case RecordType::TEXTTYPE: return "TEXTTYPE";
    case RecordType::PRESENTATION: return "PRESENTATION";
    case RecordType::STRING: return "STRING";
    case RecordType::STRANS: return "STRANS";
    case RecordType::MAG: return "MAG";
    case RecordType::ANGLE: return "ANGLE";
    case RecordType::REFLIBS: return "REFLIBS";
    case RecordType::FONTS: return "FONTS";
    case RecordType::PATHTYPE: return "PATHTYPE";
    case RecordType::GENERATIONS: return "GENERATIONS";
    case RecordType::ATTRTABLE: return "ATTRTABLE";
    case RecordType::STRCLASS: return "STRCLASS";
    case RecordType::ELFLAGS: return "ELFLAGS";
    case RecordType::NODETYPE: return "NODETYPE";
    case RecordType::PROPATTR: return "PROPATTR";
    case RecordType::PROPVALUE: return "PROPVALUE";
    case RecordType::BOX: return "BOX";
    case RecordType::BOXTYPE: return "BOXTYPE";
    case RecordType::PLEX: return "PLEX";
    case RecordType::FORMAT: return "FORMAT";
    case RecordType::MASK: return "MASK";
    case RecordType::ENDMASKS: return "ENDMASKS";
    }
    return "UNKNOWN";
}

const char* toString(DataType type) {
    switch (type) {
    case DataType::NoData: return "NoData";
    case DataType::BitArray: return "BitArray";
    case DataType::Int2: return "Int2";
    case DataType::Int4: return "Int4";
    case DataType::Real4: return "Real4";
    case DataType::Real8: return "Real8";
    case DataType::Ascii: return "Ascii";
    }
    return "UnknownDataType";
}
// Payload decoding helpers for GDS primitive data encodings.
std::vector<std::int16_t> decodeInt2Array(const Record& record) {
    if (record.header.dataType != DataType::Int2) {
        throw GdsError(std::string("Record ") + toString(record.header.recordType) + " is not Int2");
    }
    if ((record.payload.size() % 2U) != 0U) {
        throw GdsError("Int2 payload size must be even");
    }
    std::vector<std::int16_t> values;
    values.reserve(record.payload.size() / 2U);
    for (std::size_t i = 0; i < record.payload.size(); i += 2U) {
        const std::uint16_t raw = (static_cast<std::uint16_t>(record.payload[i]) << 8U) |
                                  static_cast<std::uint16_t>(record.payload[i + 1]);
        values.push_back(static_cast<std::int16_t>(raw));
    }
    return values;
}

std::vector<std::int32_t> decodeInt4Array(const Record& record) {
    if (record.header.dataType != DataType::Int4) {
        throw GdsError(std::string("Record ") + toString(record.header.recordType) + " is not Int4");
    }
    if ((record.payload.size() % 4U) != 0U) {
        throw GdsError("Int4 payload size must be divisible by 4");
    }
    std::vector<std::int32_t> values;
    values.reserve(record.payload.size() / 4U);
    for (std::size_t i = 0; i < record.payload.size(); i += 4U) {
        const std::uint32_t raw = (static_cast<std::uint32_t>(record.payload[i]) << 24U) |
                                  (static_cast<std::uint32_t>(record.payload[i + 1]) << 16U) |
                                  (static_cast<std::uint32_t>(record.payload[i + 2]) << 8U) |
                                  static_cast<std::uint32_t>(record.payload[i + 3]);
        values.push_back(static_cast<std::int32_t>(raw));
    }
    return values;
}

std::string decodeAsciiString(const Record& record) {
    if (record.header.dataType != DataType::Ascii) {
        throw GdsError(std::string("Record ") + toString(record.header.recordType) + " is not ASCII");
    }
    std::string value(record.payload.begin(), record.payload.end());
    if (!value.empty() && value.back() == '\0') {
        value.pop_back();
    }
    return value;
}
// Decodes GDS 8-byte real format:
// - bit 7 of byte 0: sign
// - lower 7 bits of byte 0: exponent with bias 64, base 16
// - bytes 1..7: fractional mantissa in base 256
double decodeReal8(const std::uint8_t* bytes) {
    bool allZero = true;
    for (int i = 0; i < 8; ++i) {
        if (bytes[i] != 0U) {
            allZero = false;
            break;
        }
    }
    if (allZero) {
        return 0.0;
    }

    const bool negative = (bytes[0] & 0x80U) != 0U;
    const int exponent = static_cast<int>(bytes[0] & 0x7FU) - 64;

    double mantissa = 0.0;
    double factor = 1.0 / 256.0;
    for (int i = 1; i < 8; ++i) {
        mantissa += static_cast<double>(bytes[i]) * factor;
        factor /= 256.0;
    }

    double value = mantissa * std::pow(16.0, static_cast<double>(exponent));
    if (negative) {
        value = -value;
    }
    return value;
}

std::vector<double> decodeReal8Array(const Record& record) {
    if (record.header.dataType != DataType::Real8) {
        throw GdsError(std::string("Record ") + toString(record.header.recordType) + " is not Real8");
    }
    if ((record.payload.size() % 8U) != 0U) {
        throw GdsError("Real8 payload size must be divisible by 8");
    }
    std::vector<double> values;
    values.reserve(record.payload.size() / 8U);
    for (std::size_t i = 0; i < record.payload.size(); i += 8U) {
        values.push_back(decodeReal8(record.payload.data() + i));
    }
    return values;
}

std::uint16_t decodeBitArrayWord(const Record& record) {
    if (record.header.dataType != DataType::BitArray) {
        throw GdsError(std::string("Record ") + toString(record.header.recordType) + " is not BitArray");
    }
    if (record.payload.size() != 2U) {
        throw GdsError("BitArray payload size must be exactly 2 bytes");
    }
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(record.payload[0]) << 8U) |
                                      static_cast<std::uint16_t>(record.payload[1]));
}

} // namespace gds::internal

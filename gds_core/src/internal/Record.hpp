#pragma once

#include "../../include/GdsTypes.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace gds::internal {

enum class DataType : std::uint8_t {
    NoData = 0,
    BitArray = 1,
    Int2 = 2,
    Int4 = 3,
    Real4 = 4,
    Real8 = 5,
    Ascii = 6,
};

enum class RecordType : std::uint8_t {
    HEADER = 0x00,
    BGNLIB = 0x01,
    LIBNAME = 0x02,
    UNITS = 0x03,
    ENDLIB = 0x04,
    BGNSTR = 0x05,
    STRNAME = 0x06,
    ENDSTR = 0x07,
    BOUNDARY = 0x08,
    PATH = 0x09,
    SREF = 0x0A,
    AREF = 0x0B,
    TEXT = 0x0C,
    LAYER = 0x0D,
    DATATYPE = 0x0E,
    WIDTH = 0x0F,
    XY = 0x10,
    ENDEL = 0x11,
    SNAME = 0x12,
    COLROW = 0x13,
    NODE = 0x15,
    TEXTTYPE = 0x16,
    PRESENTATION = 0x17,
    STRING = 0x19,
    STRANS = 0x1A,
    MAG = 0x1B,
    ANGLE = 0x1C,
    REFLIBS = 0x1F,
    FONTS = 0x20,
    PATHTYPE = 0x21,
    GENERATIONS = 0x22,
    ATTRTABLE = 0x23,
    STRCLASS = 0x34,
    ELFLAGS = 0x26,
    NODETYPE = 0x2A,
    PROPATTR = 0x2B,
    PROPVALUE = 0x2C,
    BOX = 0x2D,
    BOXTYPE = 0x2E,
    PLEX = 0x2F,
    FORMAT = 0x36,
    MASK = 0x37,
    ENDMASKS = 0x38,
};

struct RecordHeader {
    std::uint16_t length{};
    RecordType recordType{};
    DataType dataType{};
};

struct Record {
    RecordHeader header;
    std::vector<std::uint8_t> payload;
};

const char* toString(RecordType type);
const char* toString(DataType type);

std::vector<std::int16_t> decodeInt2Array(const Record& record);
std::vector<std::int32_t> decodeInt4Array(const Record& record);
std::string decodeAsciiString(const Record& record);
double decodeReal8(const std::uint8_t* bytes);
std::vector<double> decodeReal8Array(const Record& record);
std::uint16_t decodeBitArrayWord(const Record& record);

} // namespace gds::internal

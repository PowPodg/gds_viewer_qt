#include "Parser.hpp"

#include <sstream>

namespace gds::internal {

namespace {

[[noreturn]] void failUnexpected(RecordType expected, RecordType actual) {
    throw GdsError(std::string("Expected ") + toString(expected) + ", got " + toString(actual));
}

[[noreturn]] void failMessage(const std::string& message) {
    throw GdsError(message);
}

} // namespace

Parser::Parser(RecordReader& reader) : reader_(&reader) {}

Parser::Parser(RecordReader& reader,
    std::optional<std::size_t> totalBytes,
    GdsReadProgressCallback onProgress,
    GdsReadProgressOptions progressOptions)
    : reader_(&reader),
    onProgress_(std::move(onProgress)),
    progressOptions_(std::move(progressOptions)) {
    progress_.totalBytes = totalBytes;
}

void Parser::emitProgress(bool force) {

    if (!onProgress_) {
        return;
    }

    progress_.bytesRead = reader_->position();
    progress_.recordsRead = reader_->recordsRead();

    const auto now = std::chrono::steady_clock::now();

    if (!hasEmittedProgress_) {
        if (!onProgress_(progress_)) {
            throw GdsError("GDS reading cancelled");
        }

        hasEmittedProgress_ = true;
        lastEmittedBytes_ = progress_.bytesRead;
        lastEmittedStage_ = progress_.stage;
        lastEmitTime_ = now;
        return;
    }

    if (!force) {
        const bool stageChanged =
            progressOptions_.emitOnStageChange &&
            (progress_.stage != lastEmittedStage_);

        const std::size_t bytesDelta = progress_.bytesRead - lastEmittedBytes_;
        const bool enoughBytes = bytesDelta >= progressOptions_.minBytesDelta;
        const bool enoughTime = (now - lastEmitTime_) >= progressOptions_.minInterval;

        if (!stageChanged && !enoughBytes && !enoughTime) {
            return;
        }
    }

    if (!onProgress_(progress_)) {
        throw GdsError("GDS reading cancelled");
    }

    lastEmittedBytes_ = progress_.bytesRead;
    lastEmittedStage_ = progress_.stage;
    lastEmitTime_ = now;
}



GdsLibrary Parser::parse() {
    progress_.stage = GdsReadProgress::Stage::ReadingLibrary;
    progress_.currentStructureName.clear();
    emitProgress(true);

    auto library = parseLibrary();
    if (!reader_->consumeTrailingZeroPadding()) {
        throw GdsError("Trailing non-zero data after ENDLIB");
    }

    progress_.stage = GdsReadProgress::Stage::Finished;
    progress_.currentStructureName.clear();
    emitProgress(true);

    return library;
}

const Record& Parser::peek() {
    if (!lookahead_.has_value()) {
        lookahead_ = reader_->read();
        emitProgress(false);
    }
    return *lookahead_;
}

Record Parser::take() {
    if (lookahead_.has_value()) {
        Record record = std::move(*lookahead_);
        lookahead_.reset();
        return record;
    }
    Record record = reader_->read();
    emitProgress(false);
    return record;
}

bool Parser::accept(RecordType type, Record* out) {
    if (peek().header.recordType != type) {
        return false;
    }
    Record record = take();
    if (out != nullptr) {
        *out = std::move(record);
    }
    return true;
}

Record Parser::expect(RecordType type) {
    const RecordType actual = peek().header.recordType;
    if (actual != type) {
        failUnexpected(type, actual);
    }
    return take();
}

GdsLibrary Parser::parseLibrary() {
    GdsLibrary library;

    library.headerVersion = parseSingleInt2(expect(RecordType::HEADER));
    library.lastModified = parseTimestampPairFirst(expect(RecordType::BGNLIB), library.lastAccessed);
    library.name = decodeAsciiString(expect(RecordType::LIBNAME));

    Record record;
    if (accept(RecordType::REFLIBS, &record)) {
        library.reflibs = splitFixedAsciiNames(record, 44);
    }
    if (accept(RecordType::FONTS, &record)) {
        library.fonts = splitFixedAsciiNames(record, 44);
    }
    if (accept(RecordType::ATTRTABLE, &record)) {
        library.attrtable = decodeAsciiString(record);
    }
    if (accept(RecordType::GENERATIONS, &record)) {
        library.generations = parseSingleInt2(record);
    }
    if (accept(RecordType::FORMAT, &record)) {
        library.format = parseSingleInt2(record);
        while (accept(RecordType::MASK, &record)) {
            library.masks.push_back(decodeAsciiString(record));
        }
        if (!library.masks.empty()) {
            expect(RecordType::ENDMASKS);
        }
    }

    {
        const auto unitsRecord = expect(RecordType::UNITS);
        const auto values = decodeReal8Array(unitsRecord);
        if (values.size() != 2U) {
            throw GdsError("UNITS must contain exactly two 8-byte reals");
        }
        library.units.dbUnitInUserUnits = values[0];
        library.units.dbUnitInMeters = values[1];
    }

    while (peek().header.recordType == RecordType::BGNSTR) {
        library.addStructure(parseStructure());
    }

    expect(RecordType::ENDLIB);
    return library;
}

GdsStructure Parser::parseStructure() {
    GdsStructure structure;
    structure.created = parseTimestampPairFirst(expect(RecordType::BGNSTR), structure.modified);
    structure.name = decodeAsciiString(expect(RecordType::STRNAME));

    progress_.stage = GdsReadProgress::Stage::ReadingStructure;
    progress_.currentStructureName = structure.name;
    emitProgress(true);

    Record record;
    if (accept(RecordType::STRCLASS, &record)) {
        structure.strclass = static_cast<std::uint16_t>(parseSingleInt2(record));
    }

    while (true) {
        const auto type = peek().header.recordType;
        if (type == RecordType::ENDSTR) {
            break;
        }
        structure.addElement(parseElement());
    }

    expect(RecordType::ENDSTR);

    ++progress_.structuresParsed;

    if (progressOptions_.emitOnStructureBoundary) {
        emitProgress(true);
    }
    else {
        emitProgress(false);
    }

    return structure;
}

GdsElement Parser::parseElement() {
    switch (peek().header.recordType) {
    case RecordType::BOUNDARY: return parseBoundary();
    case RecordType::PATH: return parsePath();
    case RecordType::SREF: return parseSRef();
    case RecordType::AREF: return parseARef();
    case RecordType::TEXT: return parseText();
    case RecordType::NODE: return parseNode();
    case RecordType::BOX: return parseBox();
    default:
        throw GdsError(std::string("Unexpected element start record: ") + toString(peek().header.recordType));
    }
}
//общий опциональный (необязательный) префикс element records:
ElementCommon Parser::parseCommonPrefix() {
    ElementCommon common;
    Record record;
    if (accept(RecordType::ELFLAGS, &record)) {
        const auto bits = decodeBitArrayWord(record);
        common.elflags = ElFlags{
            .externalData = (bits & 0x0002U) != 0U,
            .templateData = (bits & 0x0001U) != 0U,
        };
    }
    if (accept(RecordType::PLEX, &record)) {
        common.plex = parseSingleInt4(record);
    }
    return common;
}
// Parses zero or more property pairs:
void Parser::parseProperties(ElementCommon& common) {
    while (peek().header.recordType == RecordType::PROPATTR) {
        const auto attr = parseSingleInt2(expect(RecordType::PROPATTR));
        const auto value = decodeAsciiString(expect(RecordType::PROPVALUE));
        common.properties.push_back(Property{attr, value});
    }
}
// Parses the optional transformation block:
void Parser::parseOptionalStrans(std::optional<StransFlags>& flags,
                                 std::optional<Magnitude>& mag,
                                 std::optional<Angle>& angle) {
    Record record;
    if (accept(RecordType::STRANS, &record)) {
        flags = parseStransFlags(record);
        if (accept(RecordType::MAG, &record)) {
            const auto values = decodeReal8Array(record);
            if (values.size() != 1U) {
                throw GdsError("MAG must contain exactly one value");
            }
            mag = Magnitude{values[0]};
        }
        if (accept(RecordType::ANGLE, &record)) {
            const auto values = decodeReal8Array(record);
            if (values.size() != 1U) {
                throw GdsError("ANGLE must contain exactly one value");
            }
            angle = Angle{values[0]};
        }
    }
}

Timestamp Parser::parseTimestampPairFirst(const Record& record, Timestamp& second) const {
    const auto values = decodeInt2Array(record);
    if (values.size() != 12U) {
        throw GdsError(std::string(toString(record.header.recordType)) + " must contain 12 Int2 values");
    }
    Timestamp first{values[0], values[1], values[2], values[3], values[4], values[5]};
    second = Timestamp{values[6], values[7], values[8], values[9], values[10], values[11]};
    return first;
}

std::int16_t Parser::parseSingleInt2(const Record& record) const {
    const auto values = decodeInt2Array(record);
    if (values.size() != 1U) {
        throw GdsError(std::string(toString(record.header.recordType)) + " must contain exactly one Int2 value");
    }
    return values[0];
}

std::int32_t Parser::parseSingleInt4(const Record& record) const {
    const auto values = decodeInt4Array(record);
    if (values.size() != 1U) {
        throw GdsError(std::string(toString(record.header.recordType)) + " must contain exactly one Int4 value");
    }
    return values[0];
}

std::vector<Point> Parser::parseXY(const Record& record) const {
    const auto values = decodeInt4Array(record);
    if ((values.size() % 2U) != 0U) {
        throw GdsError("XY must contain an even number of Int4 values");
    }
    std::vector<Point> points;
    points.reserve(values.size() / 2U);
    for (std::size_t i = 0; i < values.size(); i += 2U) {
        points.push_back(Point{values[i], values[i + 1]});
    }
    return points;
}

Presentation Parser::parsePresentation(const Record& record) const {
    const auto bits = decodeBitArrayWord(record);
    return Presentation{
        static_cast<std::uint8_t>((bits >> 4U) & 0x3U),
        static_cast<std::uint8_t>((bits >> 2U) & 0x3U),
        static_cast<std::uint8_t>(bits & 0x3U),
    };
}

StransFlags Parser::parseStransFlags(const Record& record) const {
    const auto bits = decodeBitArrayWord(record);
    return StransFlags{
        .reflected = (bits & 0x8000U) != 0U,
        .absoluteMagnification = (bits & 0x0004U) != 0U,
        .absoluteAngle = (bits & 0x0002U) != 0U,
    };
}
// REFLIBS and FONTS payloads are stored as fixed-width ASCII name slots.
// Each slot is read independently and trimmed at the first '\0'.
std::vector<std::string> Parser::splitFixedAsciiNames(const Record& record, std::size_t chunkSize) const {
    if (record.header.dataType != DataType::Ascii) {
        throw GdsError(std::string(toString(record.header.recordType)) + " must be ASCII");
    }
    if (chunkSize == 0U || (record.payload.size() % chunkSize) != 0U) {
        throw GdsError(std::string(toString(record.header.recordType)) + " has invalid fixed-width payload size");
    }
    std::vector<std::string> result;
    for (std::size_t offset = 0; offset < record.payload.size(); offset += chunkSize) {
        std::string item;
        for (std::size_t i = 0; i < chunkSize; ++i) {
            const char c = static_cast<char>(record.payload[offset + i]);
            if (c == '\0') {
                break;
            }
            item.push_back(c);
        }
        if (!item.empty()) {
            result.push_back(std::move(item));
        }
    }
    return result;
}
// ---------- Element parsers: polygonal geometry --------
Boundary Parser::parseBoundary() {
    expect(RecordType::BOUNDARY);
    Boundary element;
    element.common = parseCommonPrefix();
    element.layer = parseSingleInt2(expect(RecordType::LAYER));
    element.datatype = parseSingleInt2(expect(RecordType::DATATYPE));
    element.xy = parseXY(expect(RecordType::XY));
    parseProperties(element.common);
    expect(RecordType::ENDEL);
    return element;
}

Path Parser::parsePath() {
    expect(RecordType::PATH);
    Path element;
    element.common = parseCommonPrefix();
    element.layer = parseSingleInt2(expect(RecordType::LAYER));
    element.datatype = parseSingleInt2(expect(RecordType::DATATYPE));

    Record record;
    if (accept(RecordType::PATHTYPE, &record)) {
        element.pathtype = parseSingleInt2(record);
    }
    if (accept(RecordType::WIDTH, &record)) {
        element.width = parseSingleInt4(record);
    }

    element.xy = parseXY(expect(RecordType::XY));
    parseProperties(element.common);
    expect(RecordType::ENDEL);
    return element;
}
// ---------- Element parsers: references ----------
SRef Parser::parseSRef() {
    expect(RecordType::SREF);
    SRef element;
    element.common = parseCommonPrefix();
    element.sname = decodeAsciiString(expect(RecordType::SNAME));
    parseOptionalStrans(element.strans, element.mag, element.angle);
    const auto points = parseXY(expect(RecordType::XY));
    if (points.size() != 1U) {
        throw GdsError("SREF XY must contain exactly 1 point");
    }
    element.xy = points.front();
    parseProperties(element.common);
    expect(RecordType::ENDEL);
    return element;
}

ARef Parser::parseARef() {
    expect(RecordType::AREF);
    ARef element;
    element.common = parseCommonPrefix();
    element.sname = decodeAsciiString(expect(RecordType::SNAME));
    parseOptionalStrans(element.strans, element.mag, element.angle);

    {
        const auto values = decodeInt2Array(expect(RecordType::COLROW));
        if (values.size() != 2U) {
            throw GdsError("COLROW must contain exactly 2 Int2 values");
        }
        element.colrow = ColRow{values[0], values[1]};
    }

    const auto points = parseXY(expect(RecordType::XY));
    if (points.size() != 3U) {
        throw GdsError("AREF XY must contain exactly 3 points");
    }
    element.origin = points[0];
    element.columnVector = points[1];
    element.rowVector = points[2];
    parseProperties(element.common);
    expect(RecordType::ENDEL);
    return element;
}
// ---------- Element parsers: text / node / box ----------
Text Parser::parseText() {
    expect(RecordType::TEXT);
    Text element;
    element.common = parseCommonPrefix();
    element.layer = parseSingleInt2(expect(RecordType::LAYER));
    element.texttype = parseSingleInt2(expect(RecordType::TEXTTYPE));

    Record record;
    if (accept(RecordType::PRESENTATION, &record)) {
        element.presentation = parsePresentation(record);
    }
    if (accept(RecordType::PATHTYPE, &record)) {
        element.pathtype = parseSingleInt2(record);
    }
    if (accept(RecordType::WIDTH, &record)) {
        element.width = parseSingleInt4(record);
    }
    parseOptionalStrans(element.strans, element.mag, element.angle);

    const auto points = parseXY(expect(RecordType::XY));
    if (points.size() != 1U) {
        throw GdsError("TEXT XY must contain exactly 1 point");
    }
    element.xy = points.front();
    element.string = decodeAsciiString(expect(RecordType::STRING));
    parseProperties(element.common);
    expect(RecordType::ENDEL);
    return element;
}

Node Parser::parseNode() {
    expect(RecordType::NODE);
    Node element;
    element.common = parseCommonPrefix();
    element.layer = parseSingleInt2(expect(RecordType::LAYER));
    element.nodetype = parseSingleInt2(expect(RecordType::NODETYPE));
    element.xy = parseXY(expect(RecordType::XY));
    parseProperties(element.common);
    expect(RecordType::ENDEL);
    return element;
}

Box Parser::parseBox() {
    expect(RecordType::BOX);
    Box element;
    element.common = parseCommonPrefix();
    element.layer = parseSingleInt2(expect(RecordType::LAYER));
    element.boxtype = parseSingleInt2(expect(RecordType::BOXTYPE));
    element.xy = parseXY(expect(RecordType::XY));
    parseProperties(element.common);
    expect(RecordType::ENDEL);
    return element;
}

} // namespace gds::internal

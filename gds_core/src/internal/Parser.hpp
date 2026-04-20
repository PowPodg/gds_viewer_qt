#pragma once

#include "../../include/GdsLibrary.hpp"
#include "../../include/GdsProgress.hpp"
#include "Record.hpp"
#include "RecordReader.hpp"

#include <optional>

namespace gds::internal {

class Parser {
public:
    explicit Parser(RecordReader& reader);

    Parser(RecordReader& reader,
        std::optional<std::size_t> totalBytes,
        GdsReadProgressCallback onProgress,
        GdsReadProgressOptions progressOptions = {});

    GdsLibrary parse();

private:
    RecordReader* reader_{};
    std::optional<Record> lookahead_;

    GdsReadProgress progress_{};
    GdsReadProgressCallback onProgress_{};
    GdsReadProgressOptions progressOptions_{};

    bool hasEmittedProgress_{ false };
    std::size_t lastEmittedBytes_{ 0 };
    GdsReadProgress::Stage lastEmittedStage_{ GdsReadProgress::Stage::ReadingLibrary };
    std::chrono::steady_clock::time_point lastEmitTime_{};

    void emitProgress(bool force = false);

    const Record& peek();
    Record take();
    bool accept(RecordType type, Record* out = nullptr);
    Record expect(RecordType type);

    GdsLibrary parseLibrary();
    GdsStructure parseStructure();
    GdsElement parseElement();

    ElementCommon parseCommonPrefix();
    void parseProperties(ElementCommon& common);
    void parseOptionalStrans(std::optional<StransFlags>& flags,
                             std::optional<Magnitude>& mag,
                             std::optional<Angle>& angle);

    Timestamp parseTimestampPairFirst(const Record& record, Timestamp& second) const;
    std::int16_t parseSingleInt2(const Record& record) const;
    std::int32_t parseSingleInt4(const Record& record) const;
    std::vector<Point> parseXY(const Record& record) const;
    Presentation parsePresentation(const Record& record) const;
    StransFlags parseStransFlags(const Record& record) const;
    std::vector<std::string> splitFixedAsciiNames(const Record& record, std::size_t chunkSize) const;

    Boundary parseBoundary();
    Path parsePath();
    SRef parseSRef();
    ARef parseARef();
    Text parseText();
    Node parseNode();
    Box parseBox();
};

} // namespace gds::internal

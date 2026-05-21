#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <optional>
#include <string>

namespace gds {

    struct GdsReadProgress {
        enum class Stage {
            ReadingLibrary,
            ReadingStructure,
            Finished
        };

        std::size_t bytesRead{};
        std::optional<std::size_t> totalBytes;
        std::size_t recordsRead{};
        std::size_t structuresParsed{};
        Stage stage{ Stage::ReadingLibrary };
        std::string currentStructureName;
    };

    using GdsReadProgressCallback = std::function<bool(const GdsReadProgress&)>;

    struct GdsReadProgressOptions {
        std::size_t minBytesDelta = 256 * 1024;
        std::chrono::milliseconds minInterval{ 33 };
        bool emitOnStructureBoundary = true;
        bool emitOnStageChange = true;
    };

} // namespace gds

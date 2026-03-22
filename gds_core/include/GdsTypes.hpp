#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace gds {

class GdsError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

struct Timestamp {
    std::int16_t year{};
    std::int16_t month{};
    std::int16_t day{};
    std::int16_t hour{};
    std::int16_t minute{};
    std::int16_t second{};
};

struct Point {
    std::int32_t x{};
    std::int32_t y{};
};

struct Units {
    double dbUnitInUserUnits{};
    double dbUnitInMeters{};

    double userUnitInMeters() const {
        if (dbUnitInUserUnits == 0.0) {
            throw GdsError("UNITS: dbUnitInUserUnits must not be zero");
        }
        return dbUnitInMeters / dbUnitInUserUnits;
    }
};

struct Property {
    std::int16_t attr{};
    std::string value;
};

struct ElFlags {
    bool externalData{};
    bool templateData{};
};

struct StransFlags {
    bool reflected{};
    bool absoluteMagnification{};
    bool absoluteAngle{};
};

struct Presentation {
    std::uint8_t font{};                  // 0..3
    std::uint8_t verticalJustification{}; // 0=top,1=middle,2=bottom
    std::uint8_t horizontalJustification{}; // 0=left,1=center,2=right
};

struct Magnitude {
    double value{1.0};
};

struct Angle {
    double degrees{};
};

struct ColRow {
    std::int16_t columns{};
    std::int16_t rows{};
};

} // namespace gds

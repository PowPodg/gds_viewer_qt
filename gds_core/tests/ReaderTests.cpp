#include "GdsReader.hpp"
#include "GdsElements.hpp"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <variant>

namespace {

[[noreturn]] void fail(const std::string& message) {
    throw std::runtime_error(message);
}

void expect(bool condition, const std::string& message) {
    if (!condition) {
        fail(message);
    }
}

template <typename T>
const T& expectElement(const gds::GdsElement& element, const char* expectedName) {
    const auto* value = std::get_if<T>(&element);
    if (value == nullptr) {
        std::ostringstream oss;
        oss << "Expected element type " << expectedName
            << ", got " << gds::elementName(element);
        fail(oss.str());
    }
    return *value;
}

void expectPoint(const gds::Point& actual, std::int32_t x, std::int32_t y, const std::string& context) {
    if (actual.x != x || actual.y != y) {
        std::ostringstream oss;
        oss << context << ": expected point (" << x << ", " << y
            << "), got (" << actual.x << ", " << actual.y << ")";
        fail(oss.str());
    }
}

void expectTimestamp(const gds::Timestamp& ts,
                     std::int16_t year,
                     std::int16_t month,
                     std::int16_t day,
                     std::int16_t hour,
                     std::int16_t minute,
                     std::int16_t second,
                     const std::string& context) {
    if (ts.year != year || ts.month != month || ts.day != day ||
        ts.hour != hour || ts.minute != minute || ts.second != second) {
        std::ostringstream oss;
        oss << context << ": unexpected timestamp "
            << ts.year << '-' << ts.month << '-' << ts.day << ' '
            << ts.hour << ':' << ts.minute << ':' << ts.second;
        fail(oss.str());
    }
}

void expectClose(double actual, double expected, double eps, const std::string& context) {
    if (std::fabs(actual - expected) > eps) {
        std::ostringstream oss;
        oss.precision(17);
        oss << context << ": expected " << expected
            << ", got " << actual;
        fail(oss.str());
    }
}

std::filesystem::path testDataPath(const std::string& fileName) {
#ifdef GDS_CORE_TEST_DATA_DIR
    return std::filesystem::path(GDS_CORE_TEST_DATA_DIR) / fileName;
#else
    return std::filesystem::path("tests/data") / fileName;
#endif
}

void verifyLibraryMetadata(const gds::GdsLibrary& library) {
    expect(library.headerVersion == 600, "HEADER version must be 600");
    expect(library.name == "lvs.db", "LIBNAME must be lvs.db");
    expect(library.reflibs.empty(), "REFLIBS must be empty");
    expect(library.fonts.empty(), "FONTS must be empty");
    expect(!library.attrtable.has_value(), "ATTRTABLE must be absent");
    expect(!library.generations.has_value(), "GENERATIONS must be absent");
    expect(!library.format.has_value(), "FORMAT must be absent");
    expect(library.masks.empty(), "MASKS must be empty");

    expectTimestamp(library.lastModified, 2025, 11, 17, 11, 53, 35, "BGNLIB modified timestamp");
    expectTimestamp(library.lastAccessed, 2025, 11, 17, 11, 53, 35, "BGNLIB access timestamp");

    expectClose(library.units.dbUnitInUserUnits, 1.0e-6, 1.0e-18, "UNITS dbUnitInUserUnits");
    expectClose(library.units.dbUnitInMeters, 1.0e-9, 1.0e-21, "UNITS dbUnitInMeters");
    expectClose(library.units.userUnitInMeters(), 1.0e-3, 1.0e-15, "derived user unit in meters");
}

void verifyStructureAndBoundaries(const gds::GdsLibrary& library) {
    expect(library.structures.size() == 1U, "Library must contain exactly one structure");

    const gds::GdsStructure* structure = library.findStructure("m1_m2");
    expect(structure != nullptr, "Structure m1_m2 must exist");
    expect(structure->name == "m1_m2", "STRNAME must be m1_m2");
    expect(!structure->strclass.has_value(), "STRCLASS must be absent");
    expectTimestamp(structure->created, 2025, 11, 17, 11, 53, 35, "BGNSTR created timestamp");
    expectTimestamp(structure->modified, 2025, 11, 17, 11, 53, 35, "BGNSTR modified timestamp");
    expect(structure->elements.size() == 4U, "Structure must contain exactly four elements");

    const auto& b0 = expectElement<gds::Boundary>(structure->elements[0], "BOUNDARY");
    expect(b0.layer == 1, "Boundary[0] layer must be 1");
    expect(b0.datatype == 0, "Boundary[0] datatype must be 0");
    expect(!b0.common.elflags.has_value(), "Boundary[0] ELFLAGS must be absent");
    expect(!b0.common.plex.has_value(), "Boundary[0] PLEX must be absent");
    expect(b0.common.properties.size() == 1U, "Boundary[0] must contain one property");
    expect(b0.common.properties[0].attr == 5, "Boundary[0] property attr must be 5");
    expect(b0.common.properties[0].value == "3", "Boundary[0] property value must be 3");
    expect(b0.xy.size() == 5U, "Boundary[0] must contain five XY points");
    expectPoint(b0.xy[0], -1600, -1700, "Boundary[0] point[0]");
    expectPoint(b0.xy[1], 1600, -1700, "Boundary[0] point[1]");
    expectPoint(b0.xy[2], 1600, 1700, "Boundary[0] point[2]");
    expectPoint(b0.xy[3], -1600, 1700, "Boundary[0] point[3]");
    expectPoint(b0.xy[4], -1600, -1700, "Boundary[0] point[4]");

    const auto& b1 = expectElement<gds::Boundary>(structure->elements[1], "BOUNDARY");
    expect(b1.layer == 2, "Boundary[1] layer must be 2");
    expect(b1.datatype == 0, "Boundary[1] datatype must be 0");
    expect(b1.common.properties.size() == 1U, "Boundary[1] must contain one property");
    expect(b1.common.properties[0].attr == 5, "Boundary[1] property attr must be 5");
    expect(b1.common.properties[0].value == "4", "Boundary[1] property value must be 4");
    expect(b1.xy.size() == 5U, "Boundary[1] must contain five XY points");
    expectPoint(b1.xy[0], -1600, -1700, "Boundary[1] point[0]");
    expectPoint(b1.xy[1], 1600, -1700, "Boundary[1] point[1]");
    expectPoint(b1.xy[2], 1600, 1700, "Boundary[1] point[2]");
    expectPoint(b1.xy[3], -1600, 1700, "Boundary[1] point[3]");
    expectPoint(b1.xy[4], -1600, -1700, "Boundary[1] point[4]");

    const auto& b2 = expectElement<gds::Boundary>(structure->elements[2], "BOUNDARY");
    expect(b2.layer == 3, "Boundary[2] layer must be 3");
    expect(b2.datatype == 0, "Boundary[2] datatype must be 0");
    expect(b2.common.properties.size() == 1U, "Boundary[2] must contain one property");
    expect(b2.common.properties[0].attr == 5, "Boundary[2] property attr must be 5");
    expect(b2.common.properties[0].value == "1", "Boundary[2] property value must be 1");
    expect(b2.xy.size() == 5U, "Boundary[2] must contain five XY points");
    expectPoint(b2.xy[0], -600, -60, "Boundary[2] point[0]");
    expectPoint(b2.xy[1], 600, -60, "Boundary[2] point[1]");
    expectPoint(b2.xy[2], 600, 60, "Boundary[2] point[2]");
    expectPoint(b2.xy[3], -600, 60, "Boundary[2] point[3]");
    expectPoint(b2.xy[4], -600, -60, "Boundary[2] point[4]");

    const auto& b3 = expectElement<gds::Boundary>(structure->elements[3], "BOUNDARY");
    expect(b3.layer == 4, "Boundary[3] layer must be 4");
    expect(b3.datatype == 0, "Boundary[3] datatype must be 0");
    expect(b3.common.properties.size() == 1U, "Boundary[3] must contain one property");
    expect(b3.common.properties[0].attr == 5, "Boundary[3] property attr must be 5");
    expect(b3.common.properties[0].value == "2", "Boundary[3] property value must be 2");
    expect(b3.xy.size() == 5U, "Boundary[3] must contain five XY points");
    expectPoint(b3.xy[0], -70, -700, "Boundary[3] point[0]");
    expectPoint(b3.xy[1], 70, -700, "Boundary[3] point[1]");
    expectPoint(b3.xy[2], 70, 700, "Boundary[3] point[2]");
    expectPoint(b3.xy[3], -70, 700, "Boundary[3] point[3]");
    expectPoint(b3.xy[4], -70, -700, "Boundary[3] point[4]");
}

void testReadFile() {
    const auto path = testDataPath("cross.gds");
    expect(std::filesystem::exists(path), "cross.gds test data file must exist");

    gds::GdsReader reader;
    const gds::GdsLibrary library = reader.readFile(path.string());

    verifyLibraryMetadata(library);
    verifyStructureAndBoundaries(library);
}

void testReadStream() {
    const auto path = testDataPath("cross.gds");
    std::ifstream input(path, std::ios::binary);
    expect(static_cast<bool>(input), "Failed to open cross.gds via std::ifstream");

    gds::GdsReader reader;
    const gds::GdsLibrary library = reader.readStream(input);

    verifyLibraryMetadata(library);
    verifyStructureAndBoundaries(library);
}

} // namespace

int main() {
    try {
        testReadFile();
        testReadStream();
        std::cout << "gds_core_tests: OK\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "gds_core_tests: FAILED: " << ex.what() << '\n';
        return 1;
    }
}

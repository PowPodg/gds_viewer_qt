#pragma once

#include "GdsTypes.hpp"

#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace gds {

struct ElementCommon {
    std::optional<ElFlags> elflags;
    std::optional<std::int32_t> plex;
    std::vector<Property> properties;
};

struct Boundary {
    ElementCommon common;
    std::int16_t layer{};
    std::int16_t datatype{};
    std::vector<Point> xy;
};

struct Path {
    ElementCommon common;
    std::int16_t layer{};
    std::int16_t datatype{};
    std::optional<std::int16_t> pathtype;
    std::optional<std::int32_t> width;
    std::vector<Point> xy;
};

struct SRef {
    ElementCommon common;
    std::string sname;
    std::optional<StransFlags> strans;
    std::optional<Magnitude> mag;
    std::optional<Angle> angle;
    Point xy{};
};

struct ARef {
    ElementCommon common;
    std::string sname;
    std::optional<StransFlags> strans;
    std::optional<Magnitude> mag;
    std::optional<Angle> angle;
    ColRow colrow{};
    Point origin{};
    Point columnVector{};
    Point rowVector{};
};

struct Text {
    ElementCommon common;
    std::int16_t layer{};
    std::int16_t texttype{};
    std::optional<Presentation> presentation;
    std::optional<std::int16_t> pathtype;
    std::optional<std::int32_t> width;
    std::optional<StransFlags> strans;
    std::optional<Magnitude> mag;
    std::optional<Angle> angle;
    Point xy{};
    std::string string;
};

struct Node {
    ElementCommon common;
    std::int16_t layer{};
    std::int16_t nodetype{};
    std::vector<Point> xy;
};

struct Box {
    ElementCommon common;
    std::int16_t layer{};
    std::int16_t boxtype{};
    std::vector<Point> xy;
};

using GdsElement = std::variant<Boundary, Path, SRef, ARef, Text, Node, Box>;

const char* elementName(const GdsElement& element);
void validateElement(const GdsElement& element);

} // namespace gds

#pragma once

#include "GdsTypes.hpp"

#include <QDialog>
#include <QString>

#include <optional>
#include <vector>

struct SceneElementInfo {
    QString elementType;
    int layer = -1;
    QString subtypeName;
    std::optional<int> subtypeValue;
    QString primaryText;
    std::vector<gds::Point> points;
    std::vector<gds::Property> properties;
};

class ElementPropertiesDialog final : public QDialog {
public:
    explicit ElementPropertiesDialog(const SceneElementInfo& info, QWidget* parent = nullptr);
};

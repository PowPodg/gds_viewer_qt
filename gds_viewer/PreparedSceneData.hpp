#pragma once

#include "GdsLibrary.hpp"

#include <QString>

#include <memory>
#include <vector>

struct PreparedSceneBatchRange {
    int beginIndex = 0;
    int endIndex = 0; // exclusive
};

struct PreparedSceneData {
    QString path;
    std::shared_ptr<gds::GdsLibrary> library;
    QString errorText;
    int structureIndex = -1;
    QString libraryName;
    QString structureName;
    int totalElements = 0;
    std::vector<PreparedSceneBatchRange> batches;
};

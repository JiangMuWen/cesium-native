// This file was generated by generate-classes.
// DO NOT EDIT THIS FILE!
#pragma once

#include "ClassStatisticsJsonHandler.h"

#include <CesiumGltf/Statistics.h>
#include <CesiumJsonReader/DictionaryJsonHandler.h>
#include <CesiumJsonReader/ExtensibleObjectJsonHandler.h>

namespace CesiumJsonReader {
class ExtensionReaderContext;
}

namespace CesiumGltf {
class StatisticsJsonHandler
    : public CesiumJsonReader::ExtensibleObjectJsonHandler {
public:
  using ValueType = Statistics;

  StatisticsJsonHandler(
      const CesiumJsonReader::ExtensionReaderContext& context) noexcept;
  void reset(IJsonHandler* pParentHandler, Statistics* pObject);

  virtual IJsonHandler* readObjectKey(const std::string_view& str) override;

protected:
  IJsonHandler* readObjectKeyStatistics(
      const std::string& objectType,
      const std::string_view& str,
      Statistics& o);

private:
  Statistics* _pObject = nullptr;
  CesiumJsonReader::
      DictionaryJsonHandler<ClassStatistics, ClassStatisticsJsonHandler>
          _classes;
};
} // namespace CesiumGltf

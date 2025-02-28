// This file was generated by generate-classes.
// DO NOT EDIT THIS FILE!
#pragma once

#include <Cesium3DTiles/Properties.h>
#include <CesiumJsonReader/DoubleJsonHandler.h>
#include <CesiumJsonReader/ExtensibleObjectJsonHandler.h>

namespace CesiumJsonReader {
class ExtensionReaderContext;
}

namespace Cesium3DTiles {
class PropertiesJsonHandler
    : public CesiumJsonReader::ExtensibleObjectJsonHandler {
public:
  using ValueType = Properties;

  PropertiesJsonHandler(
      const CesiumJsonReader::ExtensionReaderContext& context) noexcept;
  void reset(IJsonHandler* pParentHandler, Properties* pObject);

  virtual IJsonHandler* readObjectKey(const std::string_view& str) override;

protected:
  IJsonHandler* readObjectKeyProperties(
      const std::string& objectType,
      const std::string_view& str,
      Properties& o);

private:
  Properties* _pObject = nullptr;
  CesiumJsonReader::DoubleJsonHandler _maximum;
  CesiumJsonReader::DoubleJsonHandler _minimum;
};
} // namespace Cesium3DTiles

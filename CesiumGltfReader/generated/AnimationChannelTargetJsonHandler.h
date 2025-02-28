// This file was generated by generate-classes.
// DO NOT EDIT THIS FILE!
#pragma once

#include <CesiumGltf/AnimationChannelTarget.h>
#include <CesiumJsonReader/ExtensibleObjectJsonHandler.h>
#include <CesiumJsonReader/IntegerJsonHandler.h>
#include <CesiumJsonReader/StringJsonHandler.h>

namespace CesiumJsonReader {
class ExtensionReaderContext;
}

namespace CesiumGltf {
class AnimationChannelTargetJsonHandler
    : public CesiumJsonReader::ExtensibleObjectJsonHandler {
public:
  using ValueType = AnimationChannelTarget;

  AnimationChannelTargetJsonHandler(
      const CesiumJsonReader::ExtensionReaderContext& context) noexcept;
  void reset(IJsonHandler* pParentHandler, AnimationChannelTarget* pObject);

  virtual IJsonHandler* readObjectKey(const std::string_view& str) override;

protected:
  IJsonHandler* readObjectKeyAnimationChannelTarget(
      const std::string& objectType,
      const std::string_view& str,
      AnimationChannelTarget& o);

private:
  AnimationChannelTarget* _pObject = nullptr;
  CesiumJsonReader::IntegerJsonHandler<int32_t> _node;
  CesiumJsonReader::StringJsonHandler _path;
};
} // namespace CesiumGltf

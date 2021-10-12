#include "decodeDraco.h"

#include "CesiumGltf/GltfWriter.h"

#include <CesiumGltf/KHR_draco_mesh_compression.h>
#include <CesiumGltf/Model.h>
#include <CesiumUtility/Tracing.h>

#include <cstddef>
#include <string>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4127 4018 4804)
#endif

#include <draco/compression/decode.h>
#include <draco/core/decoder_buffer.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace {
using namespace CesiumGltf;

std::unique_ptr<draco::Mesh> decodeBufferViewToDracoMesh(
    ModelWriterResult& writeModel,
    MeshPrimitive& /* primitive */,
    const KHR_draco_mesh_compression& draco) {
  CESIUM_TRACE("CesiumGltf::decodeBufferViewToDracoMesh");
  Model& model = writeModel.model.value();

  BufferView* pBufferView =
      Model::getSafe(&model.bufferViews, draco.bufferView);
  if (!pBufferView) {
    writeModel.warnings.emplace_back("Draco bufferView index is invalid.");
    return nullptr;
  }

  const BufferView& bufferView = *pBufferView;

  Buffer* pBuffer = Model::getSafe(&model.buffers, bufferView.buffer);
  if (!pBuffer) {
    writeModel.warnings.emplace_back(
        "Draco bufferView has an invalid buffer index.");
    return nullptr;
  }

  Buffer& buffer = *pBuffer;

  if (bufferView.byteOffset < 0 || bufferView.byteLength < 0 ||
      bufferView.byteOffset + bufferView.byteLength >
          static_cast<int64_t>(buffer.cesium.data.size())) {
    writeModel.warnings.emplace_back(
        "Draco bufferView extends beyond its buffer.");
    return nullptr;
  }

  const gsl::span<const std::byte> data(
      buffer.cesium.data.data() + bufferView.byteOffset,
      static_cast<uint64_t>(bufferView.byteLength));

  draco::DecoderBuffer decodeBuffer;
  decodeBuffer.Init(reinterpret_cast<const char*>(data.data()), data.size());

  draco::Decoder decoder;
  draco::Mesh mesh;
  draco::StatusOr<std::unique_ptr<draco::Mesh>> result =
      decoder.DecodeMeshFromBuffer(&decodeBuffer);
  if (!result.ok()) {
    writeModel.warnings.emplace_back(
        std::string("Draco decoding failed: ") +
        result.status().error_msg_string());
    return nullptr;
  }

  return std::move(result).value();
}

template <typename TSource, typename TDestination>
void copyData(
    const TSource* pSource,
    TDestination* pDestination,
    int64_t length) {
  std::transform(pSource, pSource + length, pDestination, [](auto x) {
    return static_cast<TDestination>(x);
  });
}

template <typename T>
void copyData(const T* pSource, T* pDestination, int64_t length) {
  std::copy(pSource, pSource + length, pDestination);
}

void copyDecodedIndices(
    ModelWriterResult& writeModel,
    const MeshPrimitive& primitive,
    draco::Mesh* pMesh) {
  CESIUM_TRACE("CesiumGltf::copyDecodedIndices");
  Model& model = writeModel.model.value();

  if (primitive.indices < 0) {
    return;
  }

  Accessor* pIndicesAccessor =
      Model::getSafe(&model.accessors, primitive.indices);
  if (!pIndicesAccessor) {
    writeModel.warnings.emplace_back(
        "Primitive indices accessor ID is invalid.");
    return;
  }

  if (pIndicesAccessor->count != pMesh->num_faces() * 3) {
    writeModel.warnings.emplace_back(
        "indices accessor doesn't match with decoded Draco indices");
    pIndicesAccessor->count = pMesh->num_faces() * 3;
  }

  draco::PointIndex::ValueType numPoint = pMesh->num_points();
  int32_t supposedComponentType = Accessor::ComponentType::UNSIGNED_BYTE;
  if (numPoint < static_cast<draco::PointIndex::ValueType>(
                     std::numeric_limits<uint8_t>::max())) {
    supposedComponentType = Accessor::ComponentType::UNSIGNED_BYTE;
  } else if (
      numPoint < static_cast<draco::PointIndex::ValueType>(
                     std::numeric_limits<uint16_t>::max())) {
    supposedComponentType = Accessor::ComponentType::UNSIGNED_SHORT;
  } else {
    supposedComponentType = Accessor::ComponentType::UNSIGNED_INT;
  }

  if (supposedComponentType > pIndicesAccessor->componentType) {
    pIndicesAccessor->componentType = supposedComponentType;
  }

  pIndicesAccessor->bufferView = static_cast<int32_t>(model.bufferViews.size());
  BufferView& indicesBufferView = model.bufferViews.emplace_back();

  indicesBufferView.buffer = static_cast<int32_t>(model.buffers.size());
  Buffer& indicesBuffer = model.buffers.emplace_back();

  int64_t indexBytes = pIndicesAccessor->computeByteSizeOfComponent();
  const int64_t indicesBytes = pIndicesAccessor->count * indexBytes;

  indicesBuffer.cesium.data.resize(static_cast<size_t>(indicesBytes));
  indicesBuffer.byteLength = indicesBytes;
  indicesBufferView.byteLength = indicesBytes;
  indicesBufferView.byteStride = indexBytes;
  indicesBufferView.byteOffset = 0;
  indicesBufferView.target = BufferView::Target::ELEMENT_ARRAY_BUFFER;
  pIndicesAccessor->type = Accessor::Type::SCALAR;

  static_assert(sizeof(draco::PointIndex) == sizeof(uint32_t));

  const uint32_t* pSourceIndices =
      reinterpret_cast<const uint32_t*>(&pMesh->face(draco::FaceIndex(0))[0]);

  switch (pIndicesAccessor->componentType) {
  case Accessor::ComponentType::BYTE:
    copyData(
        pSourceIndices,
        reinterpret_cast<int8_t*>(indicesBuffer.cesium.data.data()),
        pIndicesAccessor->count);
    break;
  case Accessor::ComponentType::UNSIGNED_BYTE:
    copyData(
        pSourceIndices,
        reinterpret_cast<uint8_t*>(indicesBuffer.cesium.data.data()),
        pIndicesAccessor->count);
    break;
  case Accessor::ComponentType::SHORT:
    copyData(
        pSourceIndices,
        reinterpret_cast<int16_t*>(indicesBuffer.cesium.data.data()),
        pIndicesAccessor->count);
    break;
  case Accessor::ComponentType::UNSIGNED_SHORT:
    copyData(
        pSourceIndices,
        reinterpret_cast<uint16_t*>(indicesBuffer.cesium.data.data()),
        pIndicesAccessor->count);
    break;
  case Accessor::ComponentType::UNSIGNED_INT:
    copyData(
        pSourceIndices,
        reinterpret_cast<uint32_t*>(indicesBuffer.cesium.data.data()),
        pIndicesAccessor->count);
    break;
  case Accessor::ComponentType::FLOAT:
    copyData(
        pSourceIndices,
        reinterpret_cast<float*>(indicesBuffer.cesium.data.data()),
        pIndicesAccessor->count);
    break;
  }
}

void copyDecodedAttribute(
    ModelWriterResult& writeModel,
    MeshPrimitive& /* primitive */,
    Accessor* pAccessor,
    const draco::Mesh* pMesh,
    const draco::PointAttribute* pAttribute) {
  CESIUM_TRACE("CesiumGltf::copyDecodedAttribute");
  Model& model = writeModel.model.value();

  if (pAccessor->count > pMesh->num_points()) {
    writeModel.warnings.emplace_back("There are fewer decoded Draco indices "
                                     "than are expected by the accessor.");

    pAccessor->count = pMesh->num_points();
  }

  pAccessor->bufferView = static_cast<int32_t>(model.bufferViews.size());
  BufferView& bufferView = model.bufferViews.emplace_back();

  bufferView.buffer = static_cast<int32_t>(model.buffers.size());
  Buffer& buffer = model.buffers.emplace_back();

  const int8_t numberOfComponents = pAccessor->computeNumberOfComponents();
  const int64_t stride =
      numberOfComponents * pAccessor->computeByteSizeOfComponent();
  const int64_t sizeBytes = pAccessor->count * stride;

  buffer.cesium.data.resize(static_cast<size_t>(sizeBytes));
  buffer.byteLength = sizeBytes;
  bufferView.byteLength = sizeBytes;
  bufferView.byteStride = stride;
  bufferView.byteOffset = 0;
  pAccessor->byteOffset = 0;

  const auto doCopy = [pMesh, pAttribute, numberOfComponents](auto pOut) {
    for (draco::PointIndex i(0); i < pMesh->num_points(); ++i) {
      const draco::AttributeValueIndex valueIndex = pAttribute->mapped_index(i);
      pAttribute->ConvertValue(valueIndex, numberOfComponents, pOut);
      pOut += pAttribute->num_components();
    }
  };

  switch (pAccessor->componentType) {
  case Accessor::ComponentType::BYTE:
    doCopy(reinterpret_cast<int8_t*>(buffer.cesium.data.data()));
    break;
  case Accessor::ComponentType::UNSIGNED_BYTE:
    doCopy(reinterpret_cast<uint8_t*>(buffer.cesium.data.data()));
    break;
  case Accessor::ComponentType::SHORT:
    doCopy(reinterpret_cast<int16_t*>(buffer.cesium.data.data()));
    break;
  case Accessor::ComponentType::UNSIGNED_SHORT:
    doCopy(reinterpret_cast<uint16_t*>(buffer.cesium.data.data()));
    break;
  case Accessor::ComponentType::UNSIGNED_INT:
    doCopy(reinterpret_cast<uint32_t*>(buffer.cesium.data.data()));
    break;
  case Accessor::ComponentType::FLOAT:
    doCopy(reinterpret_cast<float*>(buffer.cesium.data.data()));
    break;
  default:
    writeModel.warnings.emplace_back(
        "Accessor uses an unknown componentType: " +
        std::to_string(int32_t(pAccessor->componentType)));
    break;
  }
}

void decodePrimitive(
    ModelWriterResult& writeModel,
    MeshPrimitive& primitive,
    KHR_draco_mesh_compression& draco) {
  CESIUM_TRACE("CesiumGltf::decodePrimitive");
  Model& model = writeModel.model.value();

  std::unique_ptr<draco::Mesh> pMesh =
      decodeBufferViewToDracoMesh(writeModel, primitive, draco);
  if (!pMesh) {
    return;
  }

  copyDecodedIndices(writeModel, primitive, pMesh.get());

  for (const std::pair<const std::string, int32_t>& attribute :
       draco.attributes) {
    auto primitiveAttrIt = primitive.attributes.find(attribute.first);
    if (primitiveAttrIt == primitive.attributes.end()) {
      // The primitive does not use this attribute. The
      // KHR_draco_mesh_compression spec says this shouldn't happen, so warn.
      writeModel.warnings.emplace_back(
          "Draco extension has the " + attribute.first +
          " attribute, but the primitive does not have that attribute.");
      continue;
    }

    const int32_t primitiveAttrIndex = primitiveAttrIt->second;
    Accessor* pAccessor = Model::getSafe(&model.accessors, primitiveAttrIndex);
    if (!pAccessor) {
      writeModel.warnings.emplace_back(
          "Primitive attribute's accessor index is invalid.");
      continue;
    }

    const int32_t dracoAttrIndex = attribute.second;
    const draco::PointAttribute* pAttribute =
        pMesh->GetAttributeByUniqueId(static_cast<uint32_t>(dracoAttrIndex));
    if (pAttribute == nullptr) {
      writeModel.warnings.emplace_back(
          "Draco attribute with unique ID " + std::to_string(dracoAttrIndex) +
          " does not exist.");
      continue;
    }

    copyDecodedAttribute(
        writeModel,
        primitive,
        pAccessor,
        pMesh.get(),
        pAttribute);
  }
}
} // namespace

namespace CesiumGltf {

void decodeDraco(CesiumGltf::ModelWriterResult& writeModel) {
  CESIUM_TRACE("CesiumGltf::decodeDraco");
  if (!writeModel.model) {
    return;
  }

  Model& model = writeModel.model.value();

  for (Mesh& mesh : model.meshes) {
    for (MeshPrimitive& primitive : mesh.primitives) {
      KHR_draco_mesh_compression* pDraco =
          primitive.getExtension<KHR_draco_mesh_compression>();
      if (!pDraco) {
        continue;
      }

      decodePrimitive(writeModel, primitive, *pDraco);
    }
  }
}

} // namespace CesiumGltf

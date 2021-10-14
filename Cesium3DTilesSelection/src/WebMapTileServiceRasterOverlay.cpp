#include "Cesium3DTilesSelection/WebMapTileServiceRasterOverlay.h"
#include "Cesium3DTilesSelection/CreditSystem.h"
#include "Cesium3DTilesSelection/QuadtreeRasterOverlayTileProvider.h"
#include "Cesium3DTilesSelection/RasterOverlayTile.h"
#include "Cesium3DTilesSelection/TilesetExternals.h"
#include "Cesium3DTilesSelection/spdlog-cesium.h"
#include "CesiumAsync/IAssetAccessor.h"
#include "CesiumAsync/IAssetResponse.h"
#include "CesiumGeospatial/GlobeRectangle.h"
#include "CesiumGeospatial/WebMercatorProjection.h"
#include "CesiumUtility/Uri.h"
#include "tinyxml2.h"
#include <cstddef>

using namespace CesiumAsync;

namespace Cesium3DTilesSelection {

class WebMapTileServiceTileProvider final
    : public QuadtreeRasterOverlayTileProvider {
public:
  WebMapTileServiceTileProvider(
      RasterOverlay& owner,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
      std::optional<Credit> credit,
      const std::shared_ptr<IPrepareRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const CesiumGeospatial::Projection& projection,
      const CesiumGeometry::QuadtreeTilingScheme& tilingScheme,
      const CesiumGeometry::Rectangle& coverageRectangle,
      const std::string& url,
      const std::vector<std::string>& subdomains,
      const std::map<std::string, std::string>& tileMatrixLabels,
      const std::pair<std::string, std::string>& service,
      const std::pair<std::string, std::string>& version,
      const std::pair<std::string, std::string>& gettile,
      uint32_t width,
      uint32_t height,
      uint32_t minimumLevel,
      uint32_t maximumLevel,
      const std::string& format,
      const std::string& layers,
      const std::string& style,
      const std::string& tileMatrixSetID)
      : QuadtreeRasterOverlayTileProvider(
            owner,
            asyncSystem,
            pAssetAccessor,
            credit,
            pPrepareRendererResources,
            pLogger,
            projection,
            tilingScheme,
            coverageRectangle,
            minimumLevel,
            maximumLevel,
            width,
            height),
        _url(url),
        _urlTemplate(""),
        _format(format),
        _layer(layers),
        _style(style),
        _tileMatrixSetID(tileMatrixSetID),
        _subdomains(
            subdomains.empty() ? std::vector<std::string>{"a", "b", "c"}
                               : subdomains),
        _tileMatrixLabels(tileMatrixLabels),
        _service(service),
        _version(version),
        _gettile(gettile),
        _pLogger(pLogger) {}

  virtual ~WebMapTileServiceTileProvider() {}

protected:
  virtual CesiumAsync::Future<LoadedRasterOverlayImage> loadQuadtreeTileImage(
      const CesiumGeometry::QuadtreeTileID& tileID) const override {
    std::string tileMatrix;
    if (this->_tileMatrixLabels.empty()) {
      tileMatrix = std::to_string(tileID.level);
    } else {
      tileMatrix = _tileMatrixLabels.at(std::to_string(tileID.level));
    }

    std::string resolvedUrl = CesiumUtility::Uri::resolve(this->_url, "", true);
    resolvedUrl = CesiumUtility::Uri::addQuery(
        resolvedUrl,
        this->_service.first,
        this->_service.second);
    resolvedUrl = CesiumUtility::Uri::addQuery(
        resolvedUrl,
        this->_version.first,
        this->_version.second);
    resolvedUrl = CesiumUtility::Uri::addQuery(
        resolvedUrl,
        this->_gettile.first,
        this->_gettile.second);
    resolvedUrl =
        CesiumUtility::Uri::addQuery(resolvedUrl, "tilematrix", tileMatrix);
    resolvedUrl =
        CesiumUtility::Uri::addQuery(resolvedUrl, "layer", this->_layer);
    resolvedUrl =
        CesiumUtility::Uri::addQuery(resolvedUrl, "style", this->_style);
    resolvedUrl = CesiumUtility::Uri::addQuery(
        resolvedUrl,
        "tilerow",
        std::to_string(tileID.computeInvertedY(this->getTilingScheme())));
    resolvedUrl = CesiumUtility::Uri::addQuery(
        resolvedUrl,
        "tilecol",
        std::to_string(tileID.x));
    resolvedUrl = CesiumUtility::Uri::addQuery(
        resolvedUrl,
        "tilematrixset",
        this->_tileMatrixSetID);
    resolvedUrl =
        CesiumUtility::Uri::addQuery(resolvedUrl, "format", this->_format);

    resolvedUrl = CesiumUtility::Uri::substituteTemplateParameters(
        resolvedUrl,
        [this, &tileID](const std::string& key) {
          if (key == "subdomain") {
            size_t subdomainIndex =
                (tileID.level + tileID.x + tileID.y) % this->_subdomains.size();
            return this->_subdomains[subdomainIndex];
          }
          return key;
        });

    LoadTileImageFromUrlOptions options;
    options.allowEmptyImages = true;
    options.rectangle = this->getTilingScheme().tileToRectangle(tileID);
    options.moreDetailAvailable = tileID.level < this->getMaximumLevel();

    //SPDLOG_LOGGER_WARN(_pLogger, resolvedUrl);

    return this->loadTileImageFromUrl(resolvedUrl, {}, std::move(options));
  }

private:
  std::string _url;
  std::string _urlTemplate;
  std::string _format;
  std::string _layer;
  std::string _style;
  std::string _tileMatrixSetID;
  std::pair<std::string, std::string> _service;
  std::pair<std::string, std::string> _version;
  std::pair<std::string, std::string> _gettile;
  std::vector<std::string> _subdomains;
  std::map<std::string, std::string> _tileMatrixLabels;
  const std::shared_ptr<spdlog::logger>& _pLogger;
};

WebMapTileServiceRasterOverlay::WebMapTileServiceRasterOverlay(
    const std::string& name,
    const std::string& url,
    const std::string& layer,
    const std::string& style,
    const std::string& tileMatrixSetID,
    const WebMapTileServiceRasterOverlayOptions& options)
    : RasterOverlay(name),
      _url(url),
      _layer(layer),
      _style(style),
      _tileMatrixSetID(tileMatrixSetID),
      _options(options) {}

WebMapTileServiceRasterOverlay::~WebMapTileServiceRasterOverlay() {}

/*
static std::optional<std::string> getAttributeString(
    const tinyxml2::XMLElement* pElement,
    const char* attributeName) {
  if (!pElement) {
    return std::nullopt;
  }

  const char* pAttrValue = pElement->Attribute(attributeName);
  if (!pAttrValue) {
    return std::nullopt;
  }

  return std::string(pAttrValue);
}


static std::optional<uint32_t> getAttributeUint32(
    const tinyxml2::XMLElement* pElement,
    const char* attributeName) {
  std::optional<std::string> s = getAttributeString(pElement, attributeName);
  if (s) {
    return std::stoul(s.value());
  }
  return std::nullopt;
}

static std::optional<double> getAttributeDouble(
    const tinyxml2::XMLElement* pElement,
    const char* attributeName) {
  std::optional<std::string> s = getAttributeString(pElement, attributeName);
  if (s) {
    return std::stod(s.value());
  }
  return std::nullopt;
}
*/
static std::optional<std::string> getFirstChildString(
    const tinyxml2::XMLElement* pElement,
    const char* childname) {
  if (!pElement) {
    return std::nullopt;
  }

  const char* pAttrValue = pElement->FirstChildElement(childname)->GetText();
  if (!pAttrValue) {
    return std::nullopt;
  }

  return std::string(pAttrValue);
}

static std::optional<uint32_t> getFirstChildUint32(
    const tinyxml2::XMLElement* pElement,
    const char* childname) {
  std::optional<std::string> s = getFirstChildString(pElement, childname);
  if (s) {
    return std::stoul(s.value());
  }
  return std::nullopt;
}

/*
static std::optional<double> getFirstChildDouble(
    const tinyxml2::XMLElement* pElement,
    const char* childname) {
  std::optional<std::string> s = getFirstChildString(pElement, childname);
  if (s) {
    return std::stod(s.value());
  }
  return std::nullopt;
}
*/

Future<std::unique_ptr<RasterOverlayTileProvider>>
WebMapTileServiceRasterOverlay::createTileProvider(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<CreditSystem>& pCreditSystem,
    const std::shared_ptr<IPrepareRendererResources>& pPrepareRendererResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    RasterOverlay* pOwner) {

  pOwner = pOwner ? pOwner : this;

  std::pair<std::string, std::string> service =
      this->_options.service.value_or(std::make_pair("SERVICE", "WMTS"));
  std::pair<std::string, std::string> version =
      this->_options.service.value_or(std::make_pair("VERSION", "1.0.0"));
  std::pair<std::string, std::string> reqMetadata =
      this->_options.service.value_or(
          std::make_pair("REQUEST", "GetCapabilities"));
  std::pair<std::string, std::string> reqTile =
      this->_options.service.value_or(std::make_pair("REQUEST", "GetTile"));

  std::string metadataUrl = CesiumUtility::Uri::resolve(this->_url, "", true);
  metadataUrl =
      CesiumUtility::Uri::addQuery(metadataUrl, service.first, service.second);
  metadataUrl =
      CesiumUtility::Uri::addQuery(metadataUrl, version.first, version.second);
  metadataUrl = CesiumUtility::Uri::addQuery(
      metadataUrl,
      reqMetadata.first,
      reqMetadata.second);

  // SPDLOG_LOGGER_WARN(pLogger, metadataUrl);

  std::optional<Credit> credit =
      this->_options.credit ? std::make_optional(pCreditSystem->createCredit(
                                  this->_options.credit.value()))
                            : std::nullopt;

  auto handleResponse = [pOwner,
                         asyncSystem,
                         pAssetAccessor,
                         credit,
                         pPrepareRendererResources,
                         pLogger,
                         service,
                         version,
                         reqTile,
                         options = this->_options,
                         url = this->_url,
                         layer = this->_layer,
                         style = this->_style,
                         tileMatrixSetID = this->_tileMatrixSetID](
                            const std::shared_ptr<IAssetRequest>& pRequest)
      -> std::unique_ptr<RasterOverlayTileProvider> {
    // get xml metadata response
    const IAssetResponse* pResponse = pRequest->response();
    if (!pResponse) {
      SPDLOG_LOGGER_ERROR(
          pLogger,
          "No response received from Tile Map Service.");
      return nullptr;
    }

    gsl::span<const std::byte> data = pResponse->data();
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError error = doc.Parse(
        reinterpret_cast<const char*>(data.data()),
        data.size_bytes());
    if (error != tinyxml2::XMLError::XML_SUCCESS) {
      SPDLOG_LOGGER_ERROR(pLogger, "Could not parse WMTS XML.");
      return nullptr;
    }

    tinyxml2::XMLElement* pRoot = doc.RootElement();
    if (!pRoot) {
      SPDLOG_LOGGER_ERROR(
          pLogger,
          "WMTS XML document does not have a root "
          "element.");
      return nullptr;
    }

    tinyxml2::XMLElement* pTileset = pRoot->FirstChildElement("Contents")
                                         ->FirstChildElement("TileMatrixSet");
    while (pTileset) {
      std::string id =
          getFirstChildString(pTileset, "ows:Identifier").value_or("");
      std::string crs =
          getFirstChildString(pTileset, "ows:SupportedCRS").value_or("");
      if (!id.empty() && !crs.empty()) {
        tinyxml2::XMLElement* pTileMatrix =
            pTileset->FirstChildElement("TileMatrix");
        while (pTileMatrix) {
          uint32_t lv =
              getFirstChildUint32(pTileMatrix, "ows:Identifier").value_or(0);
          lv = 1;

          pTileMatrix = pTileMatrix->NextSiblingElement("TileMatrix");
        }
      }

      pTileset = pTileset->NextSiblingElement("TileMatrixSet");
    }

    CesiumGeospatial::Projection projection =
        CesiumGeospatial::WebMercatorProjection();
    CesiumGeospatial::GlobeRectangle tilingSchemeRectangle =
        CesiumGeospatial::WebMercatorProjection::MAXIMUM_GLOBE_RECTANGLE;

    uint32_t rootTilesX = 1;
    CesiumGeometry::Rectangle rectangle =
        projectRectangleSimple(projection, tilingSchemeRectangle);
    CesiumGeometry::QuadtreeTilingScheme tilingScheme(
        projectRectangleSimple(projection, tilingSchemeRectangle),
        rootTilesX,
        1);

    uint32_t minimumLevel = std::numeric_limits<uint32_t>::max();
    uint32_t maximumLevel = 0;
    minimumLevel = options.minimumLevel.value_or(minimumLevel);
    maximumLevel = options.maximumLevel.value_or(maximumLevel);

    if (maximumLevel < minimumLevel && maximumLevel == 0) {
      // Min and max levels unknown, so use defaults.
      minimumLevel = 0;
      maximumLevel = 25;
    }

    minimumLevel = glm::min(minimumLevel, maximumLevel);
    uint32_t tileWidth = options.tileWidth.value_or(256);
    uint32_t tileHeight = options.tileHeight.value_or(256);

    std::vector<std::string> subdomain =
        options.subdomains.value_or(std::vector<std::string>());
    std::map<std::string, std::string> tileMatrixLabels =
        options.tileMatrixLabels.value_or(std::map<std::string, std::string>());

    return std::make_unique<WebMapTileServiceTileProvider>(
        *pOwner,
        asyncSystem,
        pAssetAccessor,
        credit,
        pPrepareRendererResources,
        pLogger,
        projection,
        tilingScheme,
        rectangle,
        url,
        subdomain,
        tileMatrixLabels,
        service,
        version,
        reqTile,
        tileWidth,
        tileHeight,
        minimumLevel,
        maximumLevel,
        "",
        layer,
        style,
        tileMatrixSetID);
  };

  return pAssetAccessor->requestAsset(asyncSystem, metadataUrl)
      .thenInMainThread(
          [pLogger,
           handleResponse](const std::shared_ptr<IAssetRequest>& pRequest)
              -> std::unique_ptr<RasterOverlayTileProvider> {
            const IAssetResponse* pResponse = pRequest->response();

            if (pResponse == nullptr) {
              SPDLOG_LOGGER_ERROR(
                  pLogger,
                  "No response received from Arcgis Maps imagery metadata "
                  "service.");
              return nullptr;
            }

            // const std::byte* responseBuffer = pResponse->data().data();
            // size_t responseSize = pResponse->data().size();

            return handleResponse(pRequest);
          });
}

} // namespace Cesium3DTilesSelection

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
#include <sstream>
#include <fstream>

using namespace CesiumAsync;

namespace Cesium3DTilesSelection {

const std::pair<std::string, std::string> WMTSRequest::service =
    std::pair<std::string, std::string>("SERVICE", "WMTS");
const std::pair<std::string, std::string> WMTSRequest::version =
    std::pair<std::string, std::string>("VERSION", "1.0.0");
const std::pair<std::string, std::string> WMTSRequest::reqMetadata =
    std::pair<std::string, std::string>("REQUEST", "GetCapabilities");
const std::pair<std::string, std::string> WMTSRequest::reqTile =
    std::pair<std::string, std::string>("REQUEST", "GetTile");

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
      const std::string& urlTemplate,
      const std::vector<std::string>& subdomains,
      const std::vector<std::string>& tileMatrixLabels,
      const std::optional<std::pair<std::string, std::string>>& token,
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
        _urlTemplate(urlTemplate),
        _format(format),
        _layer(layers),
        _style(style),
        _tileMatrixSetID(tileMatrixSetID),
        _subdomains(subdomains),
        _tileMatrixLabels(tileMatrixLabels),
        _token(token)
  {

  }

  virtual ~WebMapTileServiceTileProvider() { 
  }

protected:
  virtual CesiumAsync::Future<LoadedRasterOverlayImage> loadQuadtreeTileImage(
      const CesiumGeometry::QuadtreeTileID& tileID) const override {
    std::string tileMatrix;
    if (this->_tileMatrixLabels.empty() ||
        this->_tileMatrixLabels.size() <= tileID.level) {
      tileMatrix = std::to_string(tileID.level);
    } else {
      tileMatrix = _tileMatrixLabels.at(tileID.level);
    }

    std::string resolvedUrl =
        CesiumUtility::Uri::resolve(this->_url, this->_urlTemplate, true);
    resolvedUrl = CesiumUtility::Uri::substituteTemplateParameters(
        resolvedUrl,
        [this, &tileID](const std::string& key) {
          if (key == "subdomain" || key == "s") {
            size_t subdomainIndex =
                (tileID.level + tileID.x + tileID.y) % this->_subdomains.size();
            return this->_subdomains[subdomainIndex];
          }
          return "{" + key + "}";
        });

    if (_urlTemplate.empty()) {
      resolvedUrl = CesiumUtility::Uri::addQuery(
          resolvedUrl,
          WMTSRequest::service.first,
          WMTSRequest::service.second);
      resolvedUrl = CesiumUtility::Uri::addQuery(
          resolvedUrl,
          WMTSRequest::version.first,
          WMTSRequest::version.second);
      resolvedUrl = CesiumUtility::Uri::addQuery(
          resolvedUrl,
          WMTSRequest::reqTile.first,
          WMTSRequest::reqTile.second);
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
      if (_token) {
        resolvedUrl = CesiumUtility::Uri::addQuery(
            resolvedUrl,
            this->_token.value().first,
            this->_token.value().second);
      }
    } else {
      resolvedUrl = CesiumUtility::Uri::substituteTemplateParameters(
          resolvedUrl,
          [this, &tileID, tileMatrix](const std::string& key) {
            if (key == "Style") {
              return this->_style;
            }
            else if (key == "TileMatrixSet") {
              return this->_tileMatrixSetID;
            }
            else if (key == "TileMatrix") {
              return tileMatrix;
            }
            else if (key == "TileRow") {
              return std::to_string(
                  tileID.computeInvertedY(this->getTilingScheme()));
            }
            else if (key == "TileCol") {
              return std::to_string(tileID.x);
            }
            else if (key == "Layer") {
              return this->_layer;
            }

            return "{" + key + "}";
          });

      if (_token) {
        resolvedUrl = CesiumUtility::Uri::substituteTemplateParameters(
            resolvedUrl,
            [this, &tileID, tileMatrix](const std::string& key) {
              if (key == this->_token.value().first) {
                return this->_token.value().second;
              }

              return "{" + key + "}";
            });
      }
    }

    LoadTileImageFromUrlOptions options;
    options.allowEmptyImages = true;
    options.rectangle = this->getTilingScheme().tileToRectangle(tileID);
    options.moreDetailAvailable = tileID.level < this->getMaximumLevel();

    return this->loadTileImageFromUrl(resolvedUrl, {}, std::move(options));
  }

private:
  std::string _url;
  std::string _urlTemplate;
  std::string _format;
  std::string _layer;
  std::string _style;
  std::string _tileMatrixSetID;
  std::optional<std::pair<std::string, std::string>> _token;
  std::vector<std::string> _subdomains;
  std::vector<std::string> _tileMatrixLabels;
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
      _urlTemplate(options.urlTemplate.value_or("")),
      _layer(layer),
      _style(style),
      _tileMatrixSetID(tileMatrixSetID),
      _options(options) {}

WebMapTileServiceRasterOverlay::~WebMapTileServiceRasterOverlay() {}

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

static std::optional<std::string>
getFirstChildString(tinyxml2::XMLElement* pElement, const char* childname) {
  if (!pElement) {
    return std::nullopt;
  }

  tinyxml2::XMLElement* fchild = pElement->FirstChildElement(childname);
  if (!fchild) {
    return std::nullopt;
  }

  const char* pAttrValue = fchild->GetText();
  if (!pAttrValue) {
    return std::nullopt;
  }

  return std::string(pAttrValue);
}

static std::optional<uint32_t>
getFirstChildUint32(tinyxml2::XMLElement* pElement, const char* childname) {
  if (!pElement) {
    return std::nullopt;
  }

  std::optional<std::string> s = getFirstChildString(pElement, childname);
  if (s) {
    return std::stoul(s.value());
  }

  return std::nullopt;
}

std::optional<std::string>
getSupportedCrs(const std::string& line, const std::string& crs) {
  if (line.empty())
    return std::nullopt;

  size_t rst = line.find(crs);
  if (rst == std::string::npos)
    return std::nullopt;
  size_t offset = rst + crs.size();
  std::string out(line.begin() + offset, line.end());
  if (out.empty())
    return std::nullopt;
  return out;
}

static std::optional<std::vector<double>>
getCorner(tinyxml2::XMLElement* pline, const char sep) {
  if (!pline)
    return std::nullopt;

  std::string line = pline->GetText();
  if (line.empty())
    return std::nullopt;
  std::vector<double> rst;
  std::string bf;
  double ele;
  auto last = line.end() - 1;
  for (auto iter = line.begin(); iter != line.end(); iter++) {
    if (*iter == sep || iter == last) {
      if (iter == last) {
        bf.push_back(*iter);
      }
      std::stringstream ss(bf);
      ss >> ele;
      rst.push_back(ele);
      bf.clear();
    } else {
      bf.push_back(*iter);
    }
  }

  if (rst.size() != 2) {
    return std::nullopt;
  }
  return rst;
}

static std::optional<std::string>
getBoundingBoxCrs(tinyxml2::XMLElement* boudingbox) {
  if (!boudingbox)
    return std::nullopt;

  std::string crs;
  std::string c1 = getAttributeString(boudingbox, "crs").value_or("");
  std::string crs1 = getSupportedCrs(c1, "crs:").value_or(c1);
  std::string c2 = getFirstChildString(boudingbox, "ows:crs").value_or("");
  std::string crs2 = getSupportedCrs(c2, "crs:").value_or(c2);

  if (!crs1.empty()) {
    return crs1;
  } else if (!crs2.empty()) {
    return crs2;
  } else {
    return std::nullopt;
  }
}

static std::optional<CesiumGeometry::Rectangle> getBoundingBoxRectangel(
    tinyxml2::XMLElement* boudingbox,
    const std::string& crs,
    bool& isRectangleInDegrees) {
  if (!boudingbox)
    return std::nullopt;

  if (crs == "EPSG::3857" || crs == "EPSG::900913" ||
      crs == "EPSG:6.18.3:3857") {
    isRectangleInDegrees = false;
  } else if (crs == "EPSG::4490" || crs == "EPSG:4490" || crs == "OGC:2:84") {
    isRectangleInDegrees = true;
  } else {
    ;
  }

  tinyxml2::XMLElement* Lowercorner =
      boudingbox->FirstChildElement("ows:LowerCorner");
  tinyxml2::XMLElement* Uppercorner =
      boudingbox->FirstChildElement("ows:UpperCorner");
  std::optional<std::vector<double>> sw = getCorner(Lowercorner, ' ');
  std::optional<std::vector<double>> ne = getCorner(Uppercorner, ' ');

  if (sw && ne) {
    return CesiumGeometry::Rectangle(
        sw.value()[0],
        sw.value()[1],
        ne.value()[0],
        ne.value()[1]);
  } else {
    return std::nullopt;
  }
}

Future<std::unique_ptr<RasterOverlayTileProvider>>
WebMapTileServiceRasterOverlay::createTileProvider(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<CreditSystem>& pCreditSystem,
    const std::shared_ptr<IPrepareRendererResources>& pPrepareRendererResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    RasterOverlay* pOwner) {

  pOwner = pOwner ? pOwner : this;

  std::string metadataUrl = CesiumUtility::Uri::resolve(this->_url, "", true);
  metadataUrl = CesiumUtility::Uri::addQuery(
      metadataUrl,
      WMTSRequest::service.first,
      WMTSRequest::service.second);
  metadataUrl = CesiumUtility::Uri::addQuery(
      metadataUrl,
      WMTSRequest::version.first,
      WMTSRequest::version.second);
  metadataUrl = CesiumUtility::Uri::addQuery(
      metadataUrl,
      WMTSRequest::reqMetadata.first,
      WMTSRequest::reqMetadata.second);
  if (_options.token) {
    metadataUrl = CesiumUtility::Uri::addQuery(
        metadataUrl,
        _options.token.value().first,
        _options.token.value().second);
  }

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
                         options = this->_options,
                         url = this->_url,
                         urlTemplate = this->_urlTemplate,
                         layer = this->_layer,
                         style = this->_style,
                         tileMatrixSetID = this->_tileMatrixSetID](
                            const std::shared_ptr<IAssetRequest>& pRequest)
      -> std::unique_ptr<RasterOverlayTileProvider> {
    // get xml metadata response
    const IAssetResponse* pResponse = pRequest->response();
    if (!pResponse) {
      SPDLOG_LOGGER_ERROR(pLogger, "No response received from WMTS.");
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

    tinyxml2::XMLElement* pContent = pRoot->FirstChildElement("Contents");
    if (!pContent) {
      SPDLOG_LOGGER_ERROR(
          pLogger,
          "WMTS XML document does not have any contents "
          "element.");
      return nullptr;
    }

    CesiumGeospatial::GlobeRectangle tilingSchemeRectangle =
        CesiumGeospatial::WebMercatorProjection::MAXIMUM_GLOBE_RECTANGLE;
    CesiumGeospatial::Projection projection =
        CesiumGeospatial::WebMercatorProjection();

    //
    uint32_t minimumLevel = std::numeric_limits<uint32_t>::max();
    uint32_t maximumLevel = 0;
    uint32_t rootTilesX = 1;
    uint32_t rootTilesY = 1;
    std::string supportedCrs = "EPSG::3857";
    tinyxml2::XMLElement* pTileset =
        pContent->FirstChildElement("TileMatrixSet");

    while (pTileset) {
      std::string id =
          getFirstChildString(pTileset, "ows:Identifier").value_or("");
      std::string crs =
          getFirstChildString(pTileset, "ows:SupportedCRS").value_or("");

      if (!id.empty() && id == tileMatrixSetID) {
        tinyxml2::XMLElement* pTileMatrix =
            pTileset->FirstChildElement("TileMatrix");
        if (!crs.empty()) {
          supportedCrs = getSupportedCrs(crs, "crs:").value_or(crs);
          if (supportedCrs == "EPSG::3857" || supportedCrs == "EPSG::900913" ||
              supportedCrs == "EPSG:6.18.3:3857") {
            tilingSchemeRectangle = CesiumGeospatial::WebMercatorProjection::
                MAXIMUM_GLOBE_RECTANGLE;
            projection = CesiumGeospatial::WebMercatorProjection();
          } else if (
              supportedCrs == "EPSG::4490" || supportedCrs == "EPSG:4490") {
            tilingSchemeRectangle =
                CesiumGeospatial::GeographicProjection::MAXIMUM_GLOBE_RECTANGLE;
            projection = CesiumGeospatial::GeographicProjection();
            rootTilesX = 2;
          }
        }
        while (pTileMatrix) {
          uint32_t lv =
              getFirstChildUint32(pTileMatrix, "ows:Identifier").value_or(0);
          if (lv > maximumLevel)
            maximumLevel = lv;
          if (lv < minimumLevel)
            minimumLevel = lv;

          pTileMatrix = pTileMatrix->NextSiblingElement("TileMatrix");
        }

        break;
      }

      pTileset = pTileset->NextSiblingElement("TileMatrixSet");
    }

    if (maximumLevel < minimumLevel && maximumLevel == 0) {
      // Min and max levels unknown, so use defaults.
      minimumLevel = 0;
      maximumLevel = 25;
    }

    //
    minimumLevel = options.minimumLevel.value_or(minimumLevel);
    maximumLevel = options.maximumLevel.value_or(maximumLevel);

    //
    CesiumGeometry::Rectangle coverageRectangle =
        CesiumGeospatial::projectRectangleSimple(
            projection,
            tilingSchemeRectangle);

    if (options.coverageRectangle) {
      coverageRectangle = options.coverageRectangle.value();
    } else {
      tinyxml2::XMLElement* pLayer = pContent->FirstChildElement("Layer");
      if (pLayer) {
        tinyxml2::XMLElement* pWGS84BoundingBox =
            pLayer->FirstChildElement("ows:WGS84BoundingBox");
        tinyxml2::XMLElement* pBoundingBox =
            pLayer->FirstChildElement("ows:BoundingBox");
        tinyxml2::XMLElement* pBBox = pLayer->FirstChildElement("BoundingBox");

        bool isRectangleInDegrees = false;
        std::optional<CesiumGeometry::Rectangle> rec;
        if (pWGS84BoundingBox) {
          std::string crs =
              getBoundingBoxCrs(pWGS84BoundingBox).value_or(supportedCrs);
          rec = getBoundingBoxRectangel(
              pWGS84BoundingBox,
              crs,
              isRectangleInDegrees);
        } else if (pBoundingBox) {
          std::string crs =
              getBoundingBoxCrs(pBoundingBox).value_or(supportedCrs);
          rec =
              getBoundingBoxRectangel(pBoundingBox, crs, isRectangleInDegrees);
        } else if (pBBox) {
          std::string crs = getBoundingBoxCrs(pBBox).value_or(supportedCrs);
          rec = getBoundingBoxRectangel(pBBox, crs, isRectangleInDegrees);
        } else {
          rec = std::nullopt;
        }

        if (rec) {
          if (isRectangleInDegrees) {
            coverageRectangle = projectRectangleSimple(
                projection,
                CesiumGeospatial::GlobeRectangle::fromDegrees(
                    rec.value().minimumX,
                    rec.value().minimumY,
                    rec.value().maximumX,
                    rec.value().maximumY));
          } else {
            coverageRectangle = rec.value();
          }
        } else {
          ;// use default coverageRectangle
        }
      }
    }

    CesiumGeometry::QuadtreeTilingScheme tilingScheme(
        projectRectangleSimple(projection, tilingSchemeRectangle),
        rootTilesX,
        rootTilesY);

    uint32_t tileWidth = options.tileWidth.value_or(256);
    uint32_t tileHeight = options.tileHeight.value_or(256);
    std::vector<std::string> subdomain =
        options.subdomains.value_or(std::vector<std::string>({"a", "b", "c"}));
    std::vector<std::string> tileMatrixLabels =
        options.tileMatrixLabels.value_or(std::vector<std::string>());

    return std::make_unique<WebMapTileServiceTileProvider>(
        *pOwner,
        asyncSystem,
        pAssetAccessor,
        credit,
        pPrepareRendererResources,
        pLogger,
        projection,
        tilingScheme,
        coverageRectangle,
        url,
        urlTemplate,
        subdomain,
        tileMatrixLabels,
        options.token,
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
                  "No response received from WMTS imagery metadata "
                  "service.");
              return nullptr;
            }

            return handleResponse(pRequest);
          });
}

} // namespace Cesium3DTilesSelection

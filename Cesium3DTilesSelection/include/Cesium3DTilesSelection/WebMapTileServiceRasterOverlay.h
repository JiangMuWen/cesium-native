#pragma once

#include "Cesium3DTilesSelection/Library.h"
#include "Cesium3DTilesSelection/RasterOverlay.h"
#include "CesiumAsync/IAssetRequest.h"
#include "CesiumGeometry/QuadtreeTilingScheme.h"
#include "CesiumGeospatial/Ellipsoid.h"
#include "CesiumGeospatial/GlobeRectangle.h"
#include "CesiumGeospatial/Projection.h"
#include <functional>
#include <memory>

namespace Cesium3DTilesSelection {

class CreditSystem;

/**
 * @brief Default parameters for WMTS requests.
 */
struct WMTSRequest {
  /**
   * @brief Service name.
   */
  static const std::pair<std::string, std::string> service;

  /**
   * @brief Service version.
   */
  static const std::pair<std::string, std::string> version;

  /**
   * @brief Get capabilities.
   */
  static const std::pair<std::string, std::string> reqMetadata;

  /**
   * @brief Get tiles.
   */
  static const std::pair<std::string, std::string> reqTile;
};

/**
 * @brief Options for web map tile service accesses.
 */
struct WebMapTileServiceRasterOverlayOptions {

  /**
   * @brief A credit for the data source, which is displayed on the canvas.
   */
  std::optional<std::string> credit;

  /**
   * @brief A list of identifiers in the TileMatrix to use for WMTS requests, one per TileMatrix level.
   */
  std::optional<std::vector<std::string>> tileMatrixLabels;

  /**
   * @brief The subdomains to use for the {s} or {subdomain} placeholder in the URL template.
   */
  std::optional<std::vector<std::string>> subdomains;

  /**
   * @brief The minimum level-of-detail supported by the imagery provider.
   *
   * Take care when specifying this that the number of tiles at the minimum
   * level is small, such as four or less. A larger number is likely to
   * result in rendering problems.
   */
  std::optional<uint32_t> minimumLevel;

  /**
   * @brief The maximum level-of-detail supported by the imagery provider.
   *
   * This will be `std::nullopt` if there is no limit.
   */
  std::optional<uint32_t> maximumLevel;

  /**
   * @brief The {@link CesiumGeometry::Rectangle}, in radians, covered by the
   * image.
   */
  std::optional<CesiumGeometry::Rectangle> coverageRectangle;

  /**
   * @brief The {@link CesiumGeospatial::Projection} that is used.
   */
  std::optional<CesiumGeospatial::Projection> projection;

  /**
   * @brief The {@link CesiumGeometry::QuadtreeTilingScheme} specifying how
   * the ellipsoidal surface is broken into tiles.
   */
  std::optional<CesiumGeometry::QuadtreeTilingScheme> tilingScheme;

  /**
   * @brief The {@link CesiumGeospatial::Ellipsoid}.
   *
   * If the `tilingScheme` is specified, this parameter is ignored and
   * the tiling scheme's ellipsoid is used instead. If neither parameter
   * is specified, the {@link CesiumGeospatial::Ellipsoid::WGS84} is used.
   */
  std::optional<CesiumGeospatial::Ellipsoid> ellipsoid;

  /**
   * @brief Pixel width of image tiles.
   */
  std::optional<uint32_t> tileWidth;

  /**
   * @brief Pixel height of image tiles.
   */
  std::optional<uint32_t> tileHeight;

  /**
   * @brief The URL template.
   */
  std::optional<std::string> urlTemplate;

  /**
   * @brief The key-value token for the query.
   */
  std::optional<std::pair<std::string, std::string>> token;


};

/**
 * @brief A {@link RasterOverlay} based on web map tile service imagery.
 */
class CESIUM3DTILESSELECTION_API WebMapTileServiceRasterOverlay final
    : public RasterOverlay {
public:
  /**
   * @brief Creates a new instance.
   *
   * @param name The user-given name of this overlay layer.
   * @param url The base URL.
   * @param layer The layer name for WMTS requests.
   * @param style The style name for WMTS requests.
   * @param tileMatrixSetID The identifier of the TileMatrixSet to use for WMTS requests.
   * @param options The {@link WebMapTileServiceRasterOverlayOptions}.
   */
  WebMapTileServiceRasterOverlay(
      const std::string& name,
      const std::string& url,
      const std::string& layer,
      const std::string& style,
      const std::string& tileMatrixSetID,
      const WebMapTileServiceRasterOverlayOptions& options =
          WebMapTileServiceRasterOverlayOptions());
  virtual ~WebMapTileServiceRasterOverlay() override;

  virtual CesiumAsync::Future<std::unique_ptr<RasterOverlayTileProvider>>
  createTileProvider(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<CreditSystem>& pCreditSystem,
      const std::shared_ptr<IPrepareRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      RasterOverlay* pOwner) override;

private:
  std::string _url;
  std::string _urlTemplate;
  std::string _layer;
  std::string _style;
  std::string _tileMatrixSetID;
  WebMapTileServiceRasterOverlayOptions _options;
};

} // namespace Cesium3DTilesSelection

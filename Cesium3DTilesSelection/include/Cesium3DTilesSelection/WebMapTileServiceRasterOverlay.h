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
 * @typedef {Object} WebMapTileServiceImageryProvider.ConstructorOptions
 *
 * Initialization options for the WebMapTileServiceImageryProvider constructor
 *
 * @property {Resource|String} url The base URL for the WMTS GetTile operation (for KVP-encoded requests) or the tile-URL template (for RESTful requests). The tile-URL template should contain the following variables: &#123;style&#125;, &#123;TileMatrixSet&#125;, &#123;TileMatrix&#125;, &#123;TileRow&#125;, &#123;TileCol&#125;. The first two are optional if actual values are hardcoded or not required by the server. The &#123;s&#125; keyword may be used to specify subdomains.
 * @property {String} [format='image/jpeg'] The MIME type for images to retrieve from the server.
 * @property {String} layer The layer name for WMTS requests.
 * @property {String} style The style name for WMTS requests.
 * @property {String} tileMatrixSetID The identifier of the TileMatrixSet to use for WMTS requests.
 * 
 * @property {Array} [tileMatrixLabels] A list of identifiers in the TileMatrix to use for WMTS requests, one per TileMatrix level.
 * @property {Clock} [clock] A Clock instance that is used when determining the value for the time dimension. Required when `times` is specified.
 * @property {TimeIntervalCollection} [times] TimeIntervalCollection with its <code>data</code> property being an object containing time dynamic dimension and their values.
 * @property {Object} [dimensions] A object containing static dimensions and their values.
 * @property {Number} [tileWidth=256] The tile width in pixels.
 * @property {Number} [tileHeight=256] The tile height in pixels.
 * @property {TilingScheme} [tilingScheme] The tiling scheme corresponding to the organization of the tiles in the TileMatrixSet.
 * @property {Rectangle} [rectangle=Rectangle.MAX_VALUE] The rectangle covered by the layer.
 * @property {Number} [minimumLevel=0] The minimum level-of-detail supported by the imagery provider.
 * @property {Number} [maximumLevel] The maximum level-of-detail supported by the imagery provider, or undefined if there is no limit.
 * @property {Ellipsoid} [ellipsoid] The ellipsoid.  If not specified, the WGS84 ellipsoid is used.
 * @property {Credit|String} [credit] A credit for the data source, which is displayed on the canvas.
 * @property {String|String[]} [subdomains='abc'] The subdomains to use for the <code>{s}</code> placeholder in the URL template.
 *                          If this parameter is a single string, each character in the string is a subdomain.  If it is
 *                          an array, each element in the array is a subdomain.
 * QuadtreeRasterOverlayTileProvider(
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
            height
            ),
 */

/**
 * @brief Options for tile map service accesses.
 */
struct WebMapTileServiceRasterOverlayOptions {

  /**
   * @brief A credit for the data source, which is displayed on the canvas.
   */
  std::optional<std::string> credit;

  std::optional<std::map<std::string, std::string>> tileMatrixLabels;

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
   * @brief Pixel height of image tiles.
   */
  std::optional<std::pair<std::string, std::string>> service;

  /**
   * @brief Pixel height of image tiles.
   */
  std::optional<std::pair<std::string, std::string>> version;

  /**
   * @brief Pixel height of image tiles.
   */
  std::optional<std::pair<std::string, std::string>> reqMetadata;

  /**
   * @brief Pixel height of image tiles.
   */
  std::optional<std::pair<std::string, std::string>> reqTile;
};

/**
 * @brief A {@link RasterOverlay} based on tile map service imagery.
 */
class CESIUM3DTILESSELECTION_API WebMapTileServiceRasterOverlay final
    : public RasterOverlay {
public:
  /**
   * @brief Creates a new instance.
   *
   * @param name The user-given name of this overlay layer.
   * @param url The base URL.
   * @param options The {@link ArcGisMapServerRasterOverlayOptions}.
   */
  WebMapTileServiceRasterOverlay(
      const std::string& name,
      const std::string& url,
      const std::string& layer,
      const std::string& style,
      const std::string& tileMatrixSetID,
      const WebMapTileServiceRasterOverlayOptions& options = WebMapTileServiceRasterOverlayOptions()
  );
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
  std::string _layer;
  std::string _style;
  std::string _tileMatrixSetID;
  WebMapTileServiceRasterOverlayOptions _options;
};

} // namespace Cesium3DTilesSelection

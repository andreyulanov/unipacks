#ifndef QGEOTILEFETCHERBINGMAPS_H
#define QGEOTILEFETCHERBINGMAPS_H

#include "qgeoserviceproviderpluginbingmaps.h"
#include <QtLocation/private/qgeotilefetcher_p.h>
#include <QMutex>
#include "krender/krender.h"

QT_BEGIN_NAMESPACE

class QGeoTiledMapReply;
class QGeoTileSpec;
class QGeoTiledMappingManagerEngine;
class QGeoTiledMappingManagerEngineBingmaps;

class QGeoTileFetcherBingmaps: public QGeoTileFetcher
{
  Q_OBJECT

public:
  QGeoTileFetcherBingmaps(const QVariantMap&,
      QGeoTiledMappingManagerEngineBingmaps* engine,
      const QSize&                           tileSize);

  QGeoTiledMapReply* getTileImage(const QGeoTileSpec& spec);

private:
  Q_DISABLE_COPY(QGeoTileFetcherBingmaps)

  QPointer<QGeoTiledMappingManagerEngineBingmaps> m_engineBingmaps;
  QSize                                           m_tileSize;

  int        _timeout;
  QByteArray _userAgent;
  QString    _language;

  // Bing version strings
  QString _versionBingMaps;
};

QT_END_NAMESPACE

#endif

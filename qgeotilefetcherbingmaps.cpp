#include "qgeotilefetcherbingmaps.h"
#include "qgeomapreplybingmaps.h"
#include "qgeotiledmapbingmaps.h"
#include "qgeotiledmappingmanagerenginebingmaps.h"
#include <QtLocation/private/qgeotilespec_p.h>

#include <QDebug>
#include <QSize>
#include <QDir>
#include <QTime>
#include "krender/krender.h"

#include <math.h>

#include <map>

QGeoTileFetcherBingmaps::QGeoTileFetcherBingmaps(
    const QVariantMap&, QGeoTiledMappingManagerEngineBingmaps* engine,
    const QSize& tileSize):
    QGeoTileFetcher(engine),
    m_engineBingmaps(engine), m_tileSize(tileSize)
{
}

QGeoTiledMapReply*
QGeoTileFetcherBingmaps::getTileImage(const QGeoTileSpec& spec)
{
  qDebug() << Q_FUNC_INFO << "m_tileSize" << m_tileSize;
  qDebug() << Q_FUNC_INFO << "spec" << spec;

  KRender::Settings s;
  s.pixel_size_mm = 0.1;
  s.window_size   = m_tileSize;
  KRender* render = new KRender(s);
  render->addPack("/home/user/unipacks/data/packs/world.kpack", true);
  render->addPack(
      "/home/user/unipacks/data/packs/ru-spe,ru-len.kpack", false);
  render->requestTile(m_tileSize.width(), spec.x(), spec.y(),
                      spec.zoom());
  QGeoTiledMapReply* mapReply =
      new QGeoMapReplyBingmaps(render, spec);

  return mapReply;
}

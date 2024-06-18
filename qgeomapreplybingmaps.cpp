#include "qgeomapreplybingmaps.h"
#include <QNetworkAccessManager>
#include <QNetworkCacheMetaData>
#include <QDateTime>
#include <QBuffer>

QT_BEGIN_NAMESPACE

QGeoMapReplyBingmaps::QGeoMapReplyBingmaps(KRender*            render,
                                           const QGeoTileSpec& spec,
                                           QObject* parent):
    QGeoTiledMapReply(spec, parent),
    m_render(render)
{
  connect(m_render, &KRender::renderedTile, this,
          &QGeoMapReplyBingmaps::renderedTile, Qt::UniqueConnection);
  connect(m_render, &KRender::busy, this,
          &QGeoMapReplyBingmaps::setBusy, Qt::UniqueConnection);
  curr_tile.x = spec.x();
  curr_tile.y = spec.y();
  curr_tile.z = spec.zoom();
}

void QGeoMapReplyBingmaps::setBusy()
{
  //  qDebug() << Q_FUNC_INFO;
  //  setFinished(false);
}

void QGeoMapReplyBingmaps::renderedTile(QPixmap pm, int x, int y,
                                        int z)
{
  if (curr_tile.x != x || curr_tile.y != y || curr_tile.z != z)
    return;

  qDebug() << Q_FUNC_INFO;

  QByteArray ba;
  QBuffer    buffer(&ba);
  buffer.open(QIODevice::WriteOnly);
  pm.save(&buffer, "bmp");
  setMapImageData(ba);
  setFinished(true);
}

QT_END_NAMESPACE

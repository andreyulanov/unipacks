#include "qgeomapreplybingmaps.h"
#include <QNetworkAccessManager>
#include <QNetworkCacheMetaData>
#include <QDateTime>

QT_BEGIN_NAMESPACE

QGeoMapReplyBingmaps::QGeoMapReplyBingmaps(KRender*            render,
                                           const QGeoTileSpec& spec,
                                           QObject* parent):
    QGeoTiledMapReply(spec, parent),
    m_render(render)
{
  connect(m_render, SIGNAL(finished(QPixmap)), this,
          SLOT(renderFinished(QPixmap)), Qt::UniqueConnection);
}

void QGeoMapReplyBingmaps::renderFinished(QPixmap pm)
{
  qDebug() << Q_FUNC_INFO;
  pm.save("/tmp/ktile.png");
  QFile      f("/tmp/ktile.png");
  QByteArray ba;
  if (f.open(QIODevice::ReadOnly))
    ba = f.readAll();
  setMapImageData(ba);
  setMapImageFormat("png");
  setFinished(true);
}

QT_END_NAMESPACE

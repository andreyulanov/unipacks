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
  connect(m_render, SIGNAL(finished(QPixmap)), this,
          SLOT(renderFinished(QPixmap)), Qt::UniqueConnection);
}

void QGeoMapReplyBingmaps::renderFinished(QPixmap pm)
{
  qDebug() << Q_FUNC_INFO;

  QByteArray ba;
  QBuffer    buffer(&ba);
  buffer.open(QIODevice::WriteOnly);
  pm.save(&buffer, "bmp");
  setMapImageData(ba);
  setFinished(true);
}

QT_END_NAMESPACE

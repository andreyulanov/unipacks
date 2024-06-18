#ifndef QGEOMAPREPLYBINGMAPS_H
#define QGEOMAPREPLYBINGMAPS_H

#include <QtNetwork/QNetworkReply>
#include <QtLocation/private/qgeotilespec_p.h>
#include <QtLocation/private/qgeotiledmapreply_p.h>
#include <QtCore/QPointer>
#include "krender/krender.h"

QT_BEGIN_NAMESPACE

class QGeoMapReplyBingmaps: public QGeoTiledMapReply
{
  Q_OBJECT

public:
  QGeoMapReplyBingmaps(QNetworkReply* reply, KRender* render,
                       const QGeoTileSpec& spec, QObject* parent = 0);
  ~QGeoMapReplyBingmaps();

  void abort();

  QNetworkReply* networkReply() const;

private Q_SLOTS:
  void networkFinished();
  void networkError(QNetworkReply::NetworkError error);
  void renderFinished(QPixmap);

private:
  QPointer<QNetworkReply> m_reply;
  QPointer<KRender>       m_render;
};

QT_END_NAMESPACE

#endif

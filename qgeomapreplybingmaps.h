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
  QGeoMapReplyBingmaps(KRender* render, const QGeoTileSpec& spec,
                       QObject* parent = 0);

private Q_SLOTS:
  void renderFinished(QPixmap);

private:
  KRender* m_render;
};

QT_END_NAMESPACE

#endif

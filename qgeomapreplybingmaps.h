#ifndef QGEOMAPREPLYBINGMAPS_H
#define QGEOMAPREPLYBINGMAPS_H

#include <QtLocation/private/qgeotilespec_p.h>
#include <QtLocation/private/qgeotiledmapreply_p.h>
#include <QtCore/QPointer>
#include "krender/krender.h"

QT_BEGIN_NAMESPACE

class QGeoMapReplyBingmaps: public QGeoTiledMapReply
{
  Q_OBJECT

  KRender*      m_render;
  KRender::Tile curr_tile;

public:
  QGeoMapReplyBingmaps(KRender* render, const QGeoTileSpec& spec,
                       QObject* parent = 0);
  void renderedTile(QPixmap, int x, int y, int z);
  void setBusy();
};

QT_END_NAMESPACE

#endif

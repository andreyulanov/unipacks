#ifndef KPACK_H
#define KPACK_H

#include <QPolygon>
#include <QColor>
#include <QPainter>
#include <QReadWriteLock>
#include <QMap>
#include <QElapsedTimer>
#include <QVariant>
#include "kobject.h"

struct KTile: public QVector<KObject>
{
  enum Status
  {
    Null,
    Loading,
    Loaded
  };
  Status status = Null;
};

struct KPack
{
  static constexpr int border_coor_precision_coef = 10000;

  double main_mip = 0;
  double tile_mip = 0;

  QVector<KClass>      classes;
  KGeoRect             frame;
  QVector<QPolygonF>   borders_m;
  QVector<KGeoPolygon> borders;
  KTile                main;
  QVector<KTile>       tiles;

  void                 save(QString path) const;
  void                 loadMain(QString path, bool load_objects,
                                double pixel_size_mm);
  void                 loadTile(QString path, int tile_idx);
  void                 loadAll(QString path, double pixel_size_mm);
  void                 clear();
  qint64               count();
  void                 addObject(KFreeObject free_obj);
  QVector<KFreeObject> getObjects();
};

#endif  // KPACK_H

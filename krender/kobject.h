#ifndef KOBJECT_H
#define KOBJECT_H

#include <QMap>
#include "kclass.h"

struct KObject
{
  int                       class_idx = 0;
  QString                   name;
  QMap<QString, QByteArray> attributes;
  KGeoRect                  frame;
  QVector<KGeoPolygon>      polygons;

public:
  void save(const QVector<KClass>& class_list, QByteArray& ba) const;
  void load(QVector<KClass>& class_list, int& pos,
            const QByteArray& ba);
  KGeoCoor getCenter();
};

struct KFreeObject: public KObject
{
  KClass cl;

  KFreeObject(KObject obj = KObject());
  void  save(QString path);
  void  load(QString path, double pixel_size_mm);
  int   getWidthPix(double pixel_size_mm);
  void  setGuid(QByteArray guid_ba);
  void  setGuid(QUuid guid);
  QUuid getGuid() const;
};

#endif  // KOBJECT_H

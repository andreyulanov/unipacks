#ifndef KCLASSMANAGER_H
#define KCLASSMANAGER_H

#include "kclass.h"

class KClassManager: public QObject
{
  double          main_mip                    = 0;
  double          tile_mip                    = 0;
  int             default_coor_precision_coef = 1;
  QString         images_dir;
  QVector<KClass> classes;

protected:
  QString error_str;

public:
  KClassManager(QString image_dir = QString());

  int    getClassIdxById(QString id);
  KClass getClassById(QString id);
  void   loadClasses(QString path, QString images_dir = QString());
  KClassImageList getClassImageList();
  QVector<KClass> getClasses();

  void   setMainMip(double);
  double getMainMip();

  void   setTileMip(double);
  double getTileMip();

  void   setDefaultCoorPrecisionCoef(double);
  double getDefaultCoorPrecisionCoef();

  QString getErrorStr();
};

#endif  // KCLASSMANAGER_H

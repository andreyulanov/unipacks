#include "kclass.h"
#include "kserialize.h"
#include <math.h>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMetaEnum>

void KClass::save(QFile* f) const
{
  using namespace KSerialize;
  write(f, id);
  write(f, type);
  write(f, style);
  write(f, layer);
  write(f, width_mm);
  write(f, min_mip);
  write(f, max_mip);
  write(f, coor_precision_coef);
  write(f, (uchar)pen.red());
  write(f, (uchar)pen.green());
  write(f, (uchar)pen.blue());
  write(f, (uchar)pen.alpha());
  write(f, (uchar)brush.red());
  write(f, (uchar)brush.green());
  write(f, (uchar)brush.blue());
  write(f, (uchar)brush.alpha());
  write(f, (uchar)tcolor.red());
  write(f, (uchar)tcolor.green());
  write(f, (uchar)tcolor.blue());
  write(f, (uchar)tcolor.alpha());
  write(f, image);
}

int KClass::getWidthPix() const
{
  return round(width_mm / pixel_size_mm);
}

void KClass::load(QFile* f, double _pixel_size_mm)
{
  using namespace KSerialize;
  pixel_size_mm = _pixel_size_mm;
  read(f, id);
  read(f, type);
  read(f, style);
  read(f, layer);
  read(f, width_mm);
  read(f, min_mip);
  read(f, max_mip);
  read(f, coor_precision_coef);
  uchar red, green, blue, alpha;
  read(f, red);
  read(f, green);
  read(f, blue);
  read(f, alpha);
  pen = QColor(red, green, blue, alpha);
  read(f, red);
  read(f, green);
  read(f, blue);
  read(f, alpha);
  brush = QColor(red, green, blue, alpha);
  read(f, red);
  read(f, green);
  read(f, blue);
  read(f, alpha);
  tcolor = QColor(red, green, blue, alpha);
  QImage img;
  read(f, img);
  if (!img.isNull())
    image =
        img.scaledToWidth(getWidthPix(), Qt::SmoothTransformation);
}

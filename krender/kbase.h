#ifndef KBASE_H
#define KBASE_H

#include <QObject>
#include <QColor>
#include <QPixmap>
#include <QFile>
#include <QPainter>
#include "kdatetime.h"

namespace kmath
{
constexpr double earth_r = 6378137;
double           deg2rad(double deg);
double           rad2deg(double rad);
double           getDistance(QPoint p1, QPoint p2);
double           getAngle(QPoint p1, QPoint p2);
double           sqr(double x);
int              getPolylinePointIdxAt(QPoint p0, QPolygon polyline,
                                       int tolerance_pix);
bool isNearPolyline(const QPoint& p0, const QPolygon& polyline,
                    int tolerance_pix);
}

class KGeoCoor
{
  friend struct KGeoRect;
  friend struct KGeoPolygon;
  int                     lat            = 0;
  int                     lon            = 0;
  static constexpr double wrap_longitude = -168.5;

public:
  KGeoCoor();
  KGeoCoor(int lat, int lon);
  static KGeoCoor fromDegs(double lat, double lon);
  static KGeoCoor fromMeters(QPointF m);
  QPointF         toMeters() const;
  double          longitude() const;
  double          latitude() const;
  bool            isValid();
  KGeoCoor        inc(KGeoCoor step) const;
  KGeoCoor        wrapped() const;
  bool            needToWrap();
};

struct KPosition
{
  KGeoCoor  coor;
  float     altitude;
  KDateTime dt;
};

struct KGeoRect
{
  KGeoCoor top_left;
  KGeoCoor bottom_right;
  KGeoRect united(const KGeoRect&) const;
  bool     isNull() const;
  QRectF   toMeters() const;
  QSizeF   getSizeMeters() const;
  QRectF   toRectM() const;
};

struct KGeoPolygon: public QVector<KGeoCoor>
{
  KGeoRect getFrame() const;
  void     save(QByteArray& ba, int coor_precision_coef) const;
  void load(const QByteArray& ba, int& pos, int coor_precision_coef);
  QPolygonF toPolygonM();
};

#endif  // KBASE_H

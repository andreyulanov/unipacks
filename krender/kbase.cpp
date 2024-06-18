#include <math.h>
#include "kbase.h"
#include "kserialize.h"

using namespace kmath;

namespace kmath
{
double deg2rad(double deg)
{
  return deg * M_PI / 180;
}
double rad2deg(double rad)
{
  return rad / M_PI * 180;
}
double sqr(double x)
{
  return x * x;
}
double getDistance(QPoint p1, QPoint p2)
{
  return sqrt(sqr(p1.x() - p2.x()) + sqr(p1.y() - p2.y()));
}
double getAngle(QPoint p1, QPoint p2)
{
  return atan2(p2.y() - p1.y(), p2.x() - p1.x());
}

int getPolylinePointIdxAt(QPoint p0, QPolygon polyline,
                          int tolerance_pix)
{
  QPoint p1 = polyline.first();
  if (getDistance(p0, p1) < tolerance_pix)
    return 0;
  int x0 = p0.x();
  int y0 = p0.y();
  for (int idx = -1; auto p2: polyline)
  {
    idx++;
    if (idx == 0)
      continue;

    if (getDistance(p0, p2) < tolerance_pix)
      return idx;

    int x1 = p1.x();
    int y1 = p1.y();
    int x2 = p2.x();
    int y2 = p2.y();

    p1 = p2;

    double l  = sqr(x2 - x1) + sqr(y2 - y1);
    double pr = (x0 - x1) * (x2 - x1) + (y0 - y1) * (y2 - y1);
    double cf = pr / l;
    if (cf < 0 || cf > 1)
      continue;

    int    xres = x1 + cf * (x2 - x1);
    int    yres = y1 + cf * (y2 - y1);
    double d    = getDistance(p0, {xres, yres});

    if (d < tolerance_pix)
      return idx;
  }
  return -1;
}

bool isNearPolyline(const QPoint& p0, const QPolygon& polyline,
                    int tolerance_pix)
{
  return getPolylinePointIdxAt(p0, polyline, tolerance_pix) >= 0;
}

}

double KGeoCoor::longitude() const
{
  return lon * 1E-7;
}

double KGeoCoor::latitude() const
{
  return -lat * 1E-7;
}

bool KGeoCoor::isValid()
{
  return !(lat == 0 && lon == 0);
}

bool KGeoCoor::needToWrap()
{
  return longitude() < wrap_longitude;
}

KGeoCoor KGeoCoor::wrapped() const
{
  if (lon > wrap_longitude * 1E+7)
    return {lat, lon};
  else
    return {lat, int(lon + 3600000000)};
}

KGeoCoor KGeoCoor::inc(KGeoCoor step) const
{
  KGeoCoor ret = *this;
  ret.lat += step.lat;
  ret.lon += step.lon;
  return ret;
}

KGeoCoor::KGeoCoor()
{
  lat = 0;
  lon = 0;
}

KGeoCoor::KGeoCoor(int _lat, int _lon)
{
  lat = _lat;
  lon = _lon;
}

KGeoCoor KGeoCoor::fromDegs(double lat, double lon)
{
  KGeoCoor ret;
  ret.lon = round(lon * 1E+7);
  ret.lat = -round(lat * 1E+7);
  return ret;
}
KGeoCoor KGeoCoor::fromMeters(QPointF m)
{
  double lon_deg = rad2deg(m.x() / earth_r);
  double lat_deg =
      -rad2deg(2 * atan(exp(m.y() / earth_r)) - M_PI / 2);
  return fromDegs(lat_deg, lon_deg);
}

QPointF KGeoCoor::toMeters() const
{
  double x = deg2rad(longitude()) * earth_r;
  double y = log(tan(deg2rad(-latitude()) / 2 + M_PI / 4)) * earth_r;
  return {x, y};
}

KGeoRect KGeoRect::united(const KGeoRect& v) const
{
  KGeoRect res;
  res.top_left.lon = std::min(top_left.lon, v.top_left.lon);
  res.top_left.lat = std::min(top_left.lat, v.top_left.lat);
  res.bottom_right.lon =
      std::max(bottom_right.lon, v.bottom_right.lon);
  res.bottom_right.lat =
      std::max(bottom_right.lat, v.bottom_right.lat);
  return res;
}

bool KGeoRect::isNull() const
{
  return top_left.lon == bottom_right.lon &&
         top_left.lat == bottom_right.lat;
}

QRectF KGeoRect::toMeters() const
{
  return {top_left.toMeters(), bottom_right.toMeters()};
}

QSizeF KGeoRect::getSizeMeters() const
{
  auto   top_left_m     = top_left.toMeters();
  auto   bottom_right_m = bottom_right.toMeters();
  double w              = bottom_right_m.x() - top_left_m.x();
  double h              = bottom_right_m.y() - top_left_m.y();
  return {w, h};
}

QRectF KGeoRect::toRectM() const
{
  return {top_left.toMeters(), bottom_right.toMeters()};
}

KGeoRect KGeoPolygon::getFrame() const
{
  using namespace std;
  auto minx = numeric_limits<int>().max();
  auto miny = numeric_limits<int>().max();
  auto maxx = numeric_limits<int>().min();
  auto maxy = numeric_limits<int>().min();
  for (auto& p: *this)
  {
    minx = min(p.lon, minx);
    miny = min(p.lat, miny);
    maxx = max(p.lon, maxx);
    maxy = max(p.lat, maxy);
  }
  KGeoRect rect;
  rect.top_left.lon     = minx;
  rect.top_left.lat     = miny;
  rect.bottom_right.lon = maxx;
  rect.bottom_right.lat = maxy;
  return rect;
}

void KGeoPolygon::save(QByteArray& ba, int coor_precision_coef) const
{
  using namespace KSerialize;
  write(ba, count());

  if (count() == 1)
  {
    write(ba, first().lat);
    write(ba, first().lon);
    return;
  }
  if (count() == 2)
  {
    write(ba, first().lat);
    write(ba, first().lon);
    write(ba, last().lat);
    write(ba, last().lon);
    return;
  }

  auto frame = getFrame();
  write(ba, frame.top_left);
  int span_lat =
      std::max(1, frame.bottom_right.lat - frame.top_left.lat);
  int span_lon =
      std::max(1, frame.bottom_right.lon - frame.top_left.lon);

  span_lat /= coor_precision_coef;
  span_lon /= coor_precision_coef;

  int span_type = 0;

  if (span_lat <= 0xff && span_lon <= 0xff)
    span_type = 0;
  else if (span_lat <= 0xffff && span_lon <= 0xffff)
    span_type = 1;
  else
    span_type = 2;

  if (span_lat == 0 || span_lon == 0)
    span_type = 2;

  write(ba, (uchar)span_type);

  if (span_type == 2)
  {
    for (auto& point: (*this))
      write(ba, point);
    return;
  }

  for (auto& point: (*this))
  {
    int dlat =
        1.0 * (point.lat - frame.top_left.lat) / coor_precision_coef;
    int dlon =
        1.0 * (point.lon - frame.top_left.lon) / coor_precision_coef;
    if (span_type == 0)
    {
      write(ba, (uchar)dlat);
      write(ba, (uchar)dlon);
    }
    else
    {
      write(ba, (ushort)dlat);
      write(ba, (ushort)dlon);
    }
  }
}

QPolygonF KGeoPolygon::toPolygonM()
{
  QPolygonF ret;
  for (auto p: *this)
    ret.append(p.toMeters());
  return ret;
}

void KGeoPolygon::load(const QByteArray& ba, int& pos,
                       int coor_precision_coef)
{
  using namespace KSerialize;
  int point_count;
  read(ba, pos, point_count);
  resize(point_count);

  if (count() <= 2)
  {
    for (auto& p: *this)
      read(ba, pos, p);
    return;
  }

  KGeoCoor top_left;
  read(ba, pos, top_left);

  uchar span_type;
  read(ba, pos, span_type);

  if (span_type == 2)
  {
    for (auto& point: (*this))
      read(ba, pos, point);
    return;
  }

  for (auto& point: (*this))
  {
    int dlat = 0;
    int dlon = 0;

    if (span_type == 0)
    {
      uchar _dlat = 0;
      uchar _dlon = 0;
      read(ba, pos, _dlat);
      read(ba, pos, _dlon);
      dlat = _dlat;
      dlon = _dlon;
    }
    else
    {
      ushort _dlat = 0;
      ushort _dlon = 0;
      read(ba, pos, _dlat);
      read(ba, pos, _dlon);
      dlat = _dlat;
      dlon = _dlon;
    }
    dlat *= coor_precision_coef;
    dlon *= coor_precision_coef;
    point.lat = top_left.lat + dlat;
    point.lon = top_left.lon + dlon;
  }
}

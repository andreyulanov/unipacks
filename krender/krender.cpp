#include "math.h"
#include "krender.h"
#include "klocker.h"
#include <QDir>
#include <QtConcurrent/QtConcurrent>
#include <QPainterPath>
#include <numeric>

using namespace kmath;

QPoint KRender::deg2scr(const KGeoCoor& deg) const
{
  return meters2pix(deg.toMeters());
}

KGeoCoor KRender::scr2deg(QPoint pix) const
{
  return KGeoCoor::fromMeters(pix2meters(pix));
}

QPoint KRender::meters2pix(QPointF coor_m) const
{
  auto rectm = getDrawRectM();
  int  pix_x = (coor_m.x() - rectm.left()) / mip;
  int  pix_y = (coor_m.y() - rectm.top()) / mip;
  return {pix_x, pix_y};
}
QPointF KRender::pix2meters(QPointF pix) const
{
  auto   rectm = getDrawRectM();
  double xm    = pix.x() * mip + rectm.left();
  double ym    = pix.y() * mip + rectm.top();
  return {xm, ym};
}

KRender::KRender(Settings v)
{
  setBackgroundColor(v.background_color);
  setPixelSizeMM(v.pixel_size_mm);
  setMaxLoadedMapsCount(v.max_loaded_maps_count);
  setPixmapSize(v.window_size);
  qRegisterMetaType<KRender::Tile>();
}

KRender::~KRender()
{
  qDebug() << Q_FUNC_INFO;
  QThreadPool().globalInstance()->waitForDone();
  stopAndWait();
}

void KRender::addPack(QString path, bool load_now)
{
  stopAndWait();
  insertPack(packs.count(), path, load_now);
}

void KRender::requestTile(Tile t)
{
  if (isRunning())
  {
    busy();
    return;
  }

  qDebug() << Q_FUNC_INFO;
  int tile_num        = pow(2, t.z);
  int world_width_pix = 256 * tile_num;
  mip                 = 1;
  mip                 = 2 * M_PI * kmath::earth_r / world_width_pix;
  center_m            = {(t.x - tile_num / 2 + 0.5) * 256 * mip,
                         (t.y - tile_num / 2 + 0.5) * 256 * mip};
  curr_tile           = t;

  qDebug() << "tile.x" << t.x;
  qDebug() << "tile.y" << t.y;
  qDebug() << "zoom" << t.z;
  qDebug() << "mip" << mip;

  render();
}

void KRender::insertPack(int idx, QString path, bool load_now)
{
  auto map = new KRenderPack(path);
  map->loadMain(load_now, pixel_size_mm);
  packs.insert(idx, map);
}

void KRender::setMip(double v)
{
  mip = v;
}

double KRender::getMip() const
{
  return mip;
}

void KRender::setCenterM(QPointF v)
{
  center_m = v;
}

QPointF KRender::getCenterM() const
{
  return center_m;
}

QPointF KRender::getRenderCenterM() const
{
  return render_center_m;
}

void KRender::setPixmapSize(QSize v)
{
  pixmap_size = v;
}

void KRender::setPixelSizeMM(double v)
{
  pixel_size_mm = v;
}

void KRender::setUpdateIntervalMs(int v)
{
  update_interval_ms = v;
}

void KRender::setBackgroundColor(QColor v)
{
  ocean_color = v;
}

double KRender::getRenderWindowSizeCoef() const
{
  return render_window_size_coef;
}

void KRender::setRenderWindowSizeCoef(double v)
{
  render_window_size_coef = v;
}

void KRender::setMaxLoadedMapsCount(int v)
{
  max_loaded_maps_count = v;
}

const QPixmap* KRender::getPixmap() const
{
  return getting_pixmap_enabled ? &render_pixmap : nullptr;
}

const KPack* KRender::getWorldPack() const
{
  return packs.first();
}

QRectF KRender::getDrawRectM() const
{
  QSizeF size_m      = {pixmap_size.width() * mip,
                        pixmap_size.height() * mip};
  QRectF draw_rect_m = {center_m.x() - size_m.width() / 2,
                        center_m.y() - size_m.height() / 2,
                        size_m.width(), size_m.height()};
  return draw_rect_m;
}

void KRender::checkUnload()
{
  auto draw_rect_m  = getDrawRectM();
  int  loaded_count = 0;
  for (int i = -1; auto& pack: packs)
  {
    i++;
    if (i == 0)
      continue;
    if (pack->main.status == KTile::Loaded)
    {
      if (!needToLoadPack(pack, draw_rect_m))
        if (loaded_count > max_loaded_maps_count)
          pack->clear();
      loaded_count++;
    }
  }
}

bool KRender::needToLoadPack(const KRenderPack* pack,
                             const QRectF&      draw_rect_m)
{
  if (pack->main_mip > 0 && render_mip > pack->main_mip)
    return false;
  auto map_rect_m       = pack->frame.toMeters();
  bool frame_intersects = draw_rect_m.intersects(map_rect_m);
  if (!frame_intersects)
    return false;

  auto top_left_m     = pix2meters({0, 0});
  auto bottom_right_m = pix2meters({(double)render_pixmap.width(),
                                    (double)render_pixmap.height()});
  auto frame_m = QRectF{top_left_m, bottom_right_m}.normalized();
  frame_m.adjust(-10000, -10000, 10000, 10000);

  QPolygonF rect;
  rect << frame_m.topLeft();
  rect << frame_m.topRight();
  rect << frame_m.bottomLeft();
  rect << frame_m.bottomRight();
  if (pack->intersects(rect))
    return true;
  return false;
}

void KRender::checkLoad()
{
  auto draw_rect_m = getDrawRectM();
  for (auto& pack: packs)
  {
    auto map_rect_m = pack->frame.toMeters();

    if (!needToLoadPack(pack, draw_rect_m))
      continue;

    if (pack->main.status == KTile::Null)
      pack->loadMain(true, pixel_size_mm);
    if (pack->main.status == KTile::Loaded)
    {
      if (needToLoadPack(pack, draw_rect_m))
      {
        int    tile_side_count = sqrt(pack->tiles.count());
        QSizeF tile_size_m = {map_rect_m.width() / tile_side_count,
                              map_rect_m.height() / tile_side_count};
        for (int tile_idx = 0; auto& tile: pack->tiles)
        {
          int    tile_idx_y = tile_idx / tile_side_count;
          int    tile_idx_x = tile_idx - tile_idx_y * tile_side_count;
          double tile_left =
              map_rect_m.x() + tile_idx_x * tile_size_m.width();
          double tile_top =
              map_rect_m.y() + tile_idx_y * tile_size_m.height();
          QRectF tile_rect_m = {{tile_left, tile_top}, tile_size_m};
          if (tile.status == KTile::Null &&
              tile_rect_m.intersects(draw_rect_m) &&
              render_mip < pack->tile_mip)
            pack->loadTile(tile_idx);
          tile_idx++;
        }
      }
    }
  }
}

void KRender::paintPointName(QPainter* p, const QString& text,
                             const QColor& tcolor)
{
  QRect rect;
  int   w = 20.0 / pixel_size_mm;
  rect.setSize({w, w});

  p->setPen(Qt::white);
  auto shifts = {-2, 0, 2};
  int  flags  = Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap |
              Qt::TextDontClip;
  for (auto x: shifts)
    for (auto y: shifts)
    {
      p->save();
      p->translate(x, y);
      p->drawText(rect, flags, text);
      p->restore();
    }

  p->setPen(tcolor);
  p->drawText(rect, flags, text);
}

void KRender::paintOutlinedText(QPainter* p, const QString& text,
                                const QColor& tcolor)
{
  p->setPen(Qt::white);
  auto shifts = {-2, 0, 2};
  for (auto x: shifts)
    for (auto y: shifts)
      p->drawText(x, y, text);

  p->setPen(tcolor);
  p->drawText(0, 0, text);
}

void KRender::paintOutlinedText(QPainter* p, const DrawTextEntry& dte)
{
  p->setPen(Qt::white);
  auto shifts = {-2, 0, 2};

  for (auto x: shifts)
    for (auto y: shifts)
    {
      p->save();
      p->translate(x, y);
      p->drawText(dte.rect,
                  dte.alignment | Qt::TextWordWrap | Qt::TextDontClip,
                  dte.text);
      p->restore();
    }

  p->setPen(dte.cl->tcolor);
  p->drawText(dte.rect,
              dte.alignment | Qt::TextWordWrap | Qt::TextDontClip,
              dte.text);
}

void KRender::addDrawTextEntry(
    QVector<DrawTextEntry>& draw_text_array, DrawTextEntry new_dte)
{
  bool can_fit = true;
  for (auto dte: draw_text_array)
  {
    if (dte.actual_rect.intersects(new_dte.actual_rect))
    {
      can_fit = false;
      break;
    }
  }
  if (can_fit)
    draw_text_array.append(new_dte);
};

QPoint KRender::deg2pix(KGeoCoor kp) const
{
  auto m = kp.toMeters();
  return {int((m.x() - render_top_left_m.x()) / render_mip),
          int((m.y() - render_top_left_m.y()) / render_mip)};
}

void KRender::paintPointObject(QPainter* p, const KRenderPack& pack,
                               const KObject& obj, int render_idx)
{
  auto frame = obj.frame;

  auto coor_m = frame.top_left.toMeters();

  if (!render_frame_m.contains(coor_m))
    return;

  auto cl = &pack.classes[obj.class_idx];
  p->setPen(QPen(cl->pen, 2));
  p->setBrush(cl->brush);
  auto        kpos       = obj.polygons.first().first();
  QPoint      pos        = deg2pix(kpos);
  int         max_length = 0;
  QStringList str_list;
  if (!obj.name.isEmpty())
  {
    str_list += obj.name;
    max_length = obj.name.count();
  }

  auto rect = QRect{pos.x(), pos.y(), max_length * cl->getWidthPix(),
                    str_list.count() * cl->getWidthPix()};

  bool intersects = false;
  for (auto item: point_names[render_idx])
  {
    if (rect.intersects(item.rect))
    {
      intersects = true;
      break;
    }
  }
  if (intersects)
    return;

  point_names[render_idx].append({rect, str_list, cl});
}

QPolygon KRender::poly2pix(const KGeoPolygon& polygon)
{
  QPoint   prev_point_pix = deg2pix(polygon.first());
  QPolygon pl;
  pl.append(prev_point_pix);
  for (int i = 1; i < polygon.count(); i++)
  {
    auto kpoint    = polygon.at(i);
    auto point_pix = deg2pix(kpoint);
    auto d         = point_pix - prev_point_pix;
    if (d.manhattanLength() > 2 || i == polygon.count() - 1)
    {
      pl.append(point_pix);
      prev_point_pix = point_pix;
    }
  }
  return pl;
}

void KRender::paintPolygonObject(QPainter* p, const KRenderPack& pack,
                                 const KObject& obj, int render_idx)
{
  auto  frame = obj.frame;
  QRect obj_frame_pix;

  auto   top_left_m     = frame.top_left.toMeters();
  auto   bottom_right_m = frame.bottom_right.toMeters();
  QRectF obj_frame_m    = {top_left_m, bottom_right_m};

  auto cl = &pack.classes[obj.class_idx];
  if (!obj_frame_m.intersects(render_frame_m))
    return;

  double obj_span_m   = sqrt(pow(obj_frame_m.width(), 2) +
                             pow(obj_frame_m.height(), 2));
  int    obj_span_pix = obj_span_m / render_mip;

  if (cl->pen == Qt::black)
    p->setPen(Qt::NoPen);
  else
    p->setPen(cl->pen);
  if (cl->style == KClass::Hatch)
    p->setBrush(QBrush(cl->brush, Qt::HorPattern));
  else if (cl->style == KClass::BDiag)
    p->setBrush(QBrush(cl->brush, Qt::BDiagPattern));
  else if (cl->style == KClass::FDiag)
    p->setBrush(QBrush(cl->brush, Qt::FDiagPattern));
  else if (cl->style == KClass::Horiz)
    p->setBrush(QBrush(cl->brush, Qt::HorPattern));
  else if (cl->style == KClass::Vert)
    p->setBrush(QBrush(cl->brush, Qt::VerPattern));
  else
    p->setBrush(cl->brush);

  QPainterPath path;
  for (int polygon_idx = -1; auto polygon: obj.polygons)
  {
    polygon_idx++;

    auto pl = poly2pix(polygon);

    if ((polygon_idx == 0 && !obj.name.isEmpty() &&
         obj_span_pix < std::min(pixmap_size.width(),
                                 pixmap_size.height() / 2) &&
         obj_span_pix >
             max_object_size_with_name_mm / pixel_size_mm) ||
        !cl->image.isNull())
    {

      QPoint top_left_pix     = deg2pix(obj.frame.top_left);
      QPoint bottom_right_pix = deg2pix(obj.frame.bottom_right);
      obj_frame_pix           = {top_left_pix, bottom_right_pix};

      auto  c = obj_frame_pix.center();
      QRect actual_rect;
      int   w = pixmap_size.width() / 32;
      actual_rect.setTopLeft(c);
      actual_rect.setSize({w, w});
      actual_rect.translate({-w / 2, -w / 2});

      addDrawTextEntry(draw_text_array[render_idx],
                       {obj.name, cl, obj_frame_pix, actual_rect,
                        Qt::AlignCenter});
    }

    if (obj.polygons.count() == 1)
    {
      p->drawPolygon(pl);
      continue;
    }

    if (polygon_idx == 0)
      p->drawPolygon(pl);
    else
    {
      p->setBrush(land_color);
      p->drawPolygon(pl);
    }
  }
}

void KRender::paintLineObject(QPainter*          painter,
                              const KRenderPack& pack,
                              const KObject& obj, int render_idx,
                              int line_iter)
{
  auto frame = obj.frame;

  auto top_left_m     = frame.top_left.toMeters();
  auto bottom_right_m = frame.bottom_right.toMeters();

  QRectF obj_frame_m = {top_left_m, bottom_right_m};

  if (!obj_frame_m.intersects(render_frame_m))
    return;

  auto cl = &pack.classes[obj.class_idx];

  Qt::PenStyle style = Qt::SolidLine;
  if (cl->style == KClass::Dash)
    style = Qt::DashLine;
  if (cl->style == KClass::Dots)
    style = Qt::DotLine;
  int obj_name_width = 0;
  if (!obj.name.isEmpty())
    obj_name_width =
        painter->font().pixelSize() * obj.name.count() * 0.3;

  auto         fixed_w    = cl->getWidthPix();
  int          sizeable_w = 0;
  int          w          = fixed_w;
  bool         one_way    = false;
  QMapIterator it(obj.attributes);
  while (it.hasNext())
  {
    it.next();
    if (it.key() == "oneway")
      one_way = true;

    if (it.key() == "lanes")
    {
      sizeable_w = 4 * it.value().toInt() / mip;
      w          = std::max((int)fixed_w, sizeable_w);
    }
  }

  painter->setPen(QPen(cl->pen, w, style));
  painter->setBrush(Qt::NoBrush);

  for (int poly_idx = -1; auto polygon: obj.polygons)
  {
    poly_idx++;
    NameHolder nh;
    QPoint     p0;
    double     a0 = 0;

    auto pl = poly2pix(polygon);

    auto size_m       = polygon.getFrame().getSizeMeters();
    auto size_pix     = (size_m.width() + size_m.height()) / mip;
    auto hatch_length = size_pix * 0.05;
    if (hatch_length > 5)
      if (cl->style == KClass::Hatch)
      {
        if (hatch_length > 5)
          hatch_length = 5;
        auto p0 = pl.first();
        for (int c = -1; auto p: pl)
        {
          c++;
          double a = 0;
          if (c == 0)
            a = getAngle(p0, pl.at(1));
          else
            a = getAngle(p0, p);
          painter->save();
          painter->translate(p);
          painter->rotate(rad2deg(a));
          int length = getDistance(p0, p);
          int step   = 10;
          for (int l = 0; l < length; l += step)
          {
            painter->drawLine(0, 0, 0, hatch_length);
            painter->translate(-step, 0);
          }
          painter->restore();
          p0 = p;
        }
      }

    if (!obj.name.isEmpty() && poly_idx == 0)
      for (int point_idx = -1; auto p: pl)
      {
        point_idx++;
        if (nh.point_count == 0)
        {
          p0 = p;
          nh.point_count++;
          continue;
        }
        auto a = getAngle(p0, p);

        if (nh.point_count == 1)
          a0 = a;
        auto da = a0 - a;
        if (da > M_PI)
          da -= 2 * M_PI;
        else if (da < -M_PI)
          da += 2 * M_PI;
        da = fabs(da);
        if (da > deg2rad(5) || point_idx == polygon.count() - 1)
        {
          if (nh.length_pix > obj_name_width)
          {
            nh.fix(&pack, &obj, pl.at(nh.start_idx),
                   pl.at(nh.end_idx));
            name_holder_array[render_idx].append(nh);
          }
          nh           = NameHolder();
          nh.start_idx = point_idx;
          p0           = p;
          continue;
        }
        auto length_pix = getDistance(p0, p);
        nh.length_pix += length_pix;
        p0 = p;
        nh.point_count++;
        nh.end_idx = point_idx;
      }

    QPen pen;
    if (line_iter == 0)
    {
      double coef = 2;
      if (w > 20)
        coef = 1.5;
      if (w > 50)
        coef = 1.2;
      int w2 = w * coef;
      pen    = QPen(cl->brush, w2, Qt::SolidLine, Qt::FlatCap,
                    Qt::RoundJoin);
    }
    else
      pen = QPen(cl->pen, w, style, Qt::FlatCap, Qt::RoundJoin);
    painter->setPen(pen);
    painter->drawPolyline(pl);
    if (sizeable_w > 7 && w > 0)
    {
      painter->setPen(Qt::black);
      auto orig_f = painter->font();
      auto f      = orig_f;
      f.setPixelSize(w);
      painter->setFont(f);
      if (pl.count() > 1 && sizeable_w > 7 && one_way)
      {
        auto p0 = pl.first();
        for (int i = 1; i < pl.count(); i++)
        {
          auto   p1  = pl.at(i);
          auto   a   = getAngle(p0, p1);
          QPoint mid = {(p0.x() + p1.x()) / 2, (p0.y() + p1.y()) / 2};
          painter->save();
          painter->translate(mid);
          painter->rotate(rad2deg(a));
          painter->drawText(0, w * 0.3, "→");
          painter->restore();
          p0 = p1;
        }
      }
      painter->setFont(orig_f);
    }
  }
}

void KRender::NameHolder::fix(const KPack* pack, const KObject* _obj,
                              const QPoint& start, const QPoint& end)
{
  obj       = _obj;
  mid_point = {(start.x() + end.x()) / 2, (start.y() + end.y()) / 2};
  angle_deg = rad2deg(getAngle(start, end));
  if (angle_deg > 90)
    angle_deg -= 180;
  if (angle_deg < -90)
    angle_deg += 180;
  tcolor = pack->classes[_obj->class_idx].tcolor;
}

bool KRender::isCluttering(const QRect& rect)
{
  bool clutter_flag = false;
  for (auto ex_rect: text_rect_array)
    if (ex_rect.intersects(rect))
    {
      clutter_flag = true;
      break;
    }
  return clutter_flag;
}

bool KRender::checkMipRange(const KPack* pack, const KObject* obj)
{
  auto cl = &pack->classes[obj->class_idx];
  return (cl->min_mip == 0 || render_mip >= cl->min_mip) &&
         (cl->max_mip == 0 || render_mip <= cl->max_mip);
}

bool KRender::paintObject(QPainter* p, const KRenderPack* map,
                          const KObject& obj, int render_idx,
                          int line_iter)
{
  auto cl = &map->classes[obj.class_idx];
  switch (cl->type)
  {
  case KClass::Point:
    paintPointObject(p, *map, obj, render_idx);
    break;
  case KClass::Line:
    paintLineObject(p, *map, obj, render_idx, line_iter);
    break;
  case KClass::Polygon:
    paintPolygonObject(p, *map, obj, render_idx);
    break;
  default:
    break;
  }
  return canContinue();
}

bool KRender::canContinue()
{
  if (!rendering_enabled)
    emit rendered(0);
  return rendering_enabled;
}

void KRender::checkYieldResult()
{
  if (auto el = yield_timer.elapsed(); el > update_interval_ms)
  {
    getting_pixmap_enabled = true;
    emit rendered(el);
    yield_timer.restart();
  }
}

bool KRender::paintPointNames(QPainter* p)
{
  for (int render_idx = 0; render_idx < KRenderPack::render_count;
       render_idx++)
    for (auto item: point_names[render_idx])
    {
      auto pos = item.rect.topLeft();
      auto w   = item.cl->getWidthPix();
      if (w > 0)
      {
        p->save();
        p->translate(pos);
        auto f = p->font();
        f.setPixelSize(w);
        p->setFont(f);
        for (auto str: item.str_list)
        {
          p->translate(
              QPoint(item.cl->image.width() * 0.8, -w * 0.3));
          paintPointName(p, str, item.cl->tcolor);
        }
        p->restore();
      }
      if (item.cl)
      {
        auto pos2 = QPoint{pos.x() - item.cl->image.width() / 2,
                           pos.y() - item.cl->image.height() / 2};
        p->drawImage(pos2, item.cl->image);
      }
      else
        p->drawEllipse(pos, int(1.0 / pixel_size_mm),
                       int(1.0 / pixel_size_mm));
      if (!canContinue())
        return false;
    }
  return true;
}

bool KRender::paintLineNames(QPainter* p)
{
  text_rect_array.clear();
  auto f = p->font();
  auto w = round(1.5 / pixel_size_mm);
  f.setPixelSize(w);
  p->setFont(f);

  for (int render_idx = 0; render_idx < KRenderPack::render_count;
       render_idx++)
    for (auto nh: name_holder_array[render_idx])
    {
      p->save();
      QRect text_rect;
      text_rect.setSize(
          {int(p->font().pixelSize() * nh.obj->name.count() * 0.6),
           p->font().pixelSize()});

      QTransform tr;
      tr.translate(nh.mid_point.x(), nh.mid_point.y());
      tr.rotate(nh.angle_deg);
      tr.translate(-text_rect.width() / 2, 0);

      QRect mapped_rect = tr.mapRect(text_rect);
      if (isCluttering(mapped_rect))
      {
        p->restore();
        continue;
      }

      p->setTransform(tr);
      text_rect_array.append(mapped_rect);
      paintOutlinedText(p, nh.obj->name, nh.tcolor);
      p->restore();
      if (!canContinue())
        return false;
    }
  return true;
}

bool KRender::paintPolygonNames(QPainter* p)
{
  for (int render_idx = 0; render_idx < KRenderPack::render_count;
       render_idx++)
    for (auto& dte: draw_text_array[render_idx])
    {
      QFontMetrics fm(p->font());
      auto         actual_rect = fm.boundingRect(
                  dte.actual_rect,
                  Qt::AlignLeft | Qt::TextWordWrap | Qt::TextDontClip,
                  dte.text);
      if (actual_rect.width() > dte.actual_rect.width() * 1.5 &&
          actual_rect.height() > dte.actual_rect.height() * 1.5)
        continue;

      if (isCluttering(actual_rect))
        continue;
      paintOutlinedText(p, dte);

      if (!dte.cl->image.isNull() &&
          dte.rect.width() > dte.cl->image.width() * 2 &&
          dte.rect.height() > dte.cl->image.height() * 2)
      {
        auto obj_center = dte.rect.center();
        auto pos =
            QPoint{obj_center.x() - dte.cl->image.width() / 2,
                   obj_center.y() - dte.cl->image.height() / 2};
        if (!dte.text.isEmpty())
          pos -= QPoint(0, dte.cl->getWidthPix() + 5);
        p->drawImage(pos, dte.cl->image);
      }

      text_rect_array.append(dte.rect);
      if (!canContinue())
        return false;
    }
  return true;
}

void KRender::render(QPainter* p, QVector<KRenderPack*> render_packs,
                     int render_idx)
{
  for (auto map: render_packs)
  {
    KLocker main_locker(&map->main_lock, KLocker::Read);
    if (!main_locker.hasLocked())
      continue;
    KLocker tile_locker(&map->tile_lock, KLocker::Read);
    if (!tile_locker.hasLocked())
      continue;

    for (int line_iter = 0; line_iter < 2; line_iter++)
      renderPack(p, map, render_idx, line_iter);
  }
}

void KRender::renderPack(QPainter* p, const KRenderPack* pack,
                         int render_idx, int line_iter)
{
  if (!pack || render_idx > pack->render_start_list.count() - 1)
    return;

  auto start        = pack->render_start_list[render_idx];
  int  object_count = 0;

  p->setRenderHint(QPainter::Antialiasing);
  for (int layer_idx = start.layer_idx;
       layer_idx < KRenderPack::max_layer_count; layer_idx++)
  {
    int start_obj_idx = 0;
    if (layer_idx == start.layer_idx)
      start_obj_idx = start.obj_idx;
    auto& layer     = pack->render_data[layer_idx];
    auto  obj_count = layer.count();

    for (int obj_idx = start_obj_idx; obj_idx < obj_count; obj_idx++)
    {
      auto obj = layer[obj_idx];
      object_count++;
      if (object_count == pack->render_object_count)
        return;
      if (!obj)
        continue;

      auto cl = &pack->classes[obj->class_idx];

      if (cl->type != KClass::Line && line_iter == 1)
        continue;

      if (cl->type == KClass::Line && line_iter == 0 &&
          cl->brush == Qt::black)
        continue;

      if (!checkMipRange(pack, obj))
        continue;

      if (!paintObject(p, pack, *obj, render_idx, line_iter))
      {
        emit rendered(0);
        return;
      }
    }
  }
}

struct RenderEntry
{
  int            idx;
  QPixmap*       pm;
  QPainter*      p;
  QElapsedTimer* t;
  QFuture<void>* fut;
  RenderEntry(int idx, QSize s, QFont* f);
  ~RenderEntry();
};

RenderEntry::RenderEntry(int _idx, QSize size, QFont* f)
{
  idx = _idx;
  pm  = new QPixmap(size);
  pm->fill(Qt::transparent);
  p = new QPainter(pm);
  p->setFont(*f);
  t = new QElapsedTimer;
  t->start();
  fut = new QFuture<void>;
}

RenderEntry::~RenderEntry()
{
  delete p;
  delete pm;
  delete t;
  delete fut;
}

void KRender::run()
{
  render_center_m = center_m;
  render_mip      = mip;

  for (int i = 0; i < KRenderPack::render_count; i++)
  {
    point_names[i].clear();
    draw_text_array[i].clear();
    name_holder_array[i].clear();
  }
  size_m            = {pixmap_size.width() * render_mip,
                       pixmap_size.height() * render_mip};
  render_top_left_m = {render_center_m.x() - size_m.width() / 2,
                       render_center_m.y() - size_m.height() / 2};
  render_frame_m    = {render_top_left_m, size_m};

  started(render_frame_m);

  if (loading_enabled)
    checkLoad();

  yield_timer.start();

  getting_pixmap_enabled = false;

  QElapsedTimer total_render_time;
  total_render_time.start();

  if (render_pixmap.size() != pixmap_size)
    render_pixmap = QPixmap(pixmap_size);

  render_pixmap.fill(ocean_color);
  QPainter p0(&render_pixmap);
  QFont    f = p0.font();

  double font_size =
      std::min((int)std::round(1.5 / pixel_size_mm / mip), 1);
  font_size =
      std::clamp(font_size, 1.5 / pixel_size_mm, 3.0 / pixel_size_mm);
  f.setPixelSize(font_size);
  f.setBold(true);
  p0.setFont(f);

  QVector<int> intersecting_packs;
  auto         draw_rect = getDrawRectM();
  for (int pack_idx = -1; auto& pack: packs)
  {
    if (pack->main_mip > 0 && render_mip > pack->main_mip)
      continue;
    pack_idx++;
    if (pack_idx == 0)
      continue;

    if (needToLoadPack(pack, draw_rect))
      intersecting_packs.append(pack_idx);
  }

  QVector<KRenderPack*> render_packs;
  for (int pack_idx = -1; auto& pack: packs)
  {
    pack_idx++;

    if (pack_idx > 0)
      if (!intersecting_packs.contains(pack_idx))
        continue;

    KLocker big_locker(&pack->main_lock, KLocker::Read);
    if (!big_locker.hasLocked())
      continue;

    if (pack_idx > 0 && !needToLoadPack(pack, render_frame_m))
      continue;

    auto pack_rect_m = pack->frame.toMeters();
    if (pack_idx > 0 && !render_frame_m.intersects(pack_rect_m))
      continue;

    KLocker small_locker(&pack->tile_lock, KLocker::Read);
    if (!small_locker.hasLocked())
      continue;

    if (pack->render_start_list.isEmpty())
      continue;

    render_packs.append(pack);
  }

  QList<RenderEntry*> render_list;
  for (int render_idx = 1; render_idx < KRenderPack::render_count;
       render_idx++)
  {
    auto render =
        new RenderEntry(render_idx, render_pixmap.size(), &f);
    *render->fut = QtConcurrent::run(
        this, &KRender::render, render->p, render_packs, render_idx);
    render_list.append(render);
  }

  render(&p0, render_packs, 0);

  for (auto render: render_list)
    render->fut->waitForFinished();

  for (auto render: render_list)
    p0.drawPixmap(0, 0, *render->pm);

  qDeleteAll(render_list);

  QElapsedTimer t;
  t.start();

  if (!paintLineNames(&p0))
  {
    emit rendered(0);
    return;
  }
  if (!paintPolygonNames(&p0))
  {
    emit rendered(0);
    return;
  }
  if (!paintPointNames(&p0))
  {
    emit rendered(0);
    return;
  }

  qDebug() << "mip" << render_mip << ",total render time elapsed"
           << total_render_time.elapsed();

  main_pixmap = render_pixmap.copy();
  paintUserObjects(&p0);

  getting_pixmap_enabled = true;
  emit rendered(0);
  if (loading_enabled)
    checkLoad();
  renderedTile(render_pixmap, curr_tile.x, curr_tile.y, curr_tile.z);
}

void KRender::renderUserObjects()
{
  if (isRunning())
    return;
  if (!getting_pixmap_enabled)
    return;
  if (time_since_last_repaint.isValid() &&
      time_since_last_repaint.elapsed() < 10)
    return;
  render_pixmap = main_pixmap.copy();
  QPainter p(&render_pixmap);
  paintUserObjects(&p);
  rendered(0);
  time_since_last_repaint.start();
}

void KRender::render()
{
  if (isRunning())
    return;
  rendering_enabled = true;
  if (QThreadPool::globalInstance()->activeThreadCount() == 0)
    checkUnload();
  QThread::start();
}

void KRender::stopAndWait()
{
  rendering_enabled = false;
  wait();
}

void KRender::enableLoading(bool v)
{
  loading_enabled = v;
}

void KRender::pan(QPoint shift_pix)
{
  enableLoading(true);
  QPointF shift_m = shift_pix * mip;
  setCenterM(center_m + shift_m);
}

void KRender::zoom(double coef)
{
  mip *= coef;
}

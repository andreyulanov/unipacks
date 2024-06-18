#include "math.h"
#include "kfreeobjectmanager.h"
#include "kserialize.h"
#include <QDir>
#include <QDebug>
#include <QUuid>

KFreeObjectManager::KFreeObjectManager(QString _objects_dir,
                                       double  _pixel_size_mm)
{
  pixel_size_mm = _pixel_size_mm;
  QDir dir(_objects_dir);
  if (!dir.exists())
    dir.mkdir(_objects_dir);
  auto fi_list = dir.entryInfoList(QDir::Files, QDir::Name);
  for (auto fi: fi_list)
  {
    KFreeObject obj;
    obj.load(fi.absoluteFilePath(), pixel_size_mm);
    auto guid = QUuid::fromString(fi.fileName().remove(".kfree"));
    obj.setGuid(guid);
    objects.append(obj);
  }
  objects_dir   = _objects_dir;
  pixel_size_mm = _pixel_size_mm;

  pack.main_mip = 200;
  pack.tile_mip = 50;
}

QString KFreeObjectManager::getObjectPath(QUuid object_guid)
{
  return objects_dir + "/" + object_guid.toString() + ".kfree";
}

void KFreeObjectManager::removeObject()
{
  if (edited_object_idx >= 0)
  {
    QFile().remove(
        getObjectPath(objects.at(edited_object_idx).getGuid()));
    objects.remove(edited_object_idx);
    edited_object_idx = -1;
  }
  for (auto guid: selected_guids)
  {
    for (int obj_idx = -1; auto& obj: objects)
    {
      obj_idx++;
      if (obj.getGuid() == guid)
      {
        QFile().remove(getObjectPath(objects.at(obj_idx).getGuid()));
        objects.remove(obj_idx);
        break;
      }
    }
  }
  selected_guids.clear();
  finishEdit();
  updated();
}

void KFreeObjectManager::createObject(KClass sh)
{
  KFreeObject obj;
  obj.cl = sh;
  objects.append(obj);
  edited_object_idx      = objects.count() - 1;
  is_creating_new_object = true;
}

void KFreeObjectManager::paintObject(QPainter* p, KFreeObject obj,
                                     PaintMode paint_mode)
{
  if (obj.polygons.isEmpty())
    return;

  auto w = obj.getWidthPix(pixel_size_mm);
  if (obj.cl.type == KClass::Point)
  {
    auto& img = obj.cl.image;
    auto  pix = deg2pix(obj.polygons.first().first());
    if (img.isNull())
      p->drawEllipse(pix, w, w);
    else
    {
      auto s = img.size();
      auto pos =
          QPoint{pix.x() - s.width() / 2, pix.y() - s.height() / 2};
      if (paint_mode != PaintMode::Normal)
      {
        p->setPen(Qt::NoPen);
        p->setBrush(Qt::yellow);
        p->drawEllipse(QRect{pos, obj.cl.image.size()}.adjusted(
            -w / 2, -w / 2, w / 2, w / 2));
      }
      p->drawImage(pos, obj.cl.image);
    }
    return;
  }

  if (obj.cl.type == KClass::Line)
  {
    for (auto polygon: obj.polygons)
    {
      int      w = obj.getWidthPix(pixel_size_mm);
      QPolygon polygon_pix;
      for (auto point: polygon)
        polygon_pix.append(deg2pix(point));
      if (paint_mode != PaintMode::Normal)
        p->setPen(QPen(Qt::yellow, w * 2, Qt::SolidLine, Qt::RoundCap,
                       Qt::RoundJoin));
      else
        p->setPen(QPen(obj.cl.pen, w, Qt::SolidLine, Qt::RoundCap,
                       Qt::RoundJoin));
      p->drawPolyline(polygon_pix);
      if (paint_mode == PaintMode::Edited)
      {
        p->setPen(Qt::white);
        p->setBrush(Qt::black);
        for (auto point: polygon_pix)
        {
          int w = 1.0 / pixel_size_mm;
          p->drawEllipse(point, w, w);
        }
      }
    }
  }
  if (obj.cl.type == KClass::Polygon)
  {
    int    w     = obj.getWidthPix(pixel_size_mm);
    QPen   pen   = QPen(obj.cl.pen, w);
    QBrush brush = QBrush(obj.cl.brush);
    for (auto polygon: obj.polygons)
    {
      QPolygon polygon_pix;
      for (auto point: polygon)
        polygon_pix.append(deg2pix(point));

      if (paint_mode != PaintMode::Normal)
        p->setPen(QPen(Qt::yellow, w * 2));
      else
        p->setPen(pen);
      p->setBrush(brush);
      if (polygon_pix.count() == 2)
        p->drawPolyline(polygon_pix);
      else
        p->drawPolygon(polygon_pix);

      if (paint_mode == PaintMode::Edited)
      {
        p->setPen(Qt::white);
        p->setBrush(Qt::black);
        for (auto point: polygon_pix)
        {
          int w = 1.0 / pixel_size_mm;
          p->drawEllipse(point, w, w);
        }
      }
    }
  }
}

void KFreeObjectManager::onTapped(KGeoCoor coor)
{
  auto p0 = deg2pix(coor);

  if (!selected_guids.isEmpty())
  {
    auto object_idx    = getObjectIdxAt(p0);
    auto selected_guid = objects.at(object_idx).getGuid();
    if (!selected_guids.contains(selected_guid))
      selected_guids.append(selected_guid);
    updated();
    return;
  }

  if (edited_object_idx >= 0)
  {
    auto& obj = objects[edited_object_idx];
    if (obj.polygons.isEmpty())
    {
      KGeoPolygon polygon;
      polygon.append(coor);
      obj.polygons.append(polygon);
      updated();
      return;
    }
    auto new_selected_object_idx = getObjectIdxAt(p0);
    if (new_selected_object_idx >= 0 &&
        new_selected_object_idx != edited_object_idx)
    {
      selected_guids.clear();
      selected_guids.append(objects.at(edited_object_idx).getGuid());
      selected_guids.append(
          objects.at(new_selected_object_idx).getGuid());
      edited_object_idx = -1;
      updated();
      return;
    }
  }

  auto proximity_pix = proximity_mm / pixel_size_mm;
  if (edited_object_idx < 0 && selected_guids.isEmpty())
  {
    edited_object_idx      = getObjectIdxAt(p0);
    is_creating_new_object = false;
    if (edited_object_idx >= 0)
    {
      startEdit();
      updated();
    }
    return;
  }
  if (edited_object_idx < 0)
    return;
  auto& obj  = objects[edited_object_idx];
  auto  type = obj.cl.type;
  if (type == KClass::Point)
  {
    KGeoPolygon poly;
    poly.append(coor);
    obj.polygons.append(poly);
    acceptObject();
    return;
  }
  else
  {
    if (obj.polygons.isEmpty())
    {
      KGeoPolygon polygon;
      polygon.append(coor);
      obj.polygons.append(polygon);
    }
    else
    {
      for (int polygon_idx = -1; auto& polygon: obj.polygons)
      {
        polygon_idx++;
        if (polygon.count() < 4 || type == KClass::Line ||
            is_creating_new_object)
        {
          polygon.append(coor);
          updated();
          return;
        }
        QPolygon polygon_pix;
        for (auto& p: polygon)
          polygon_pix.append(deg2pix(p));
        polygon_pix.append(deg2pix(polygon.first()));
        auto point_idx = kmath::getPolylinePointIdxAt(p0, polygon_pix,
                                                      proximity_pix);
        if (point_idx >= 0)
        {
          polygon.insert(point_idx, coor);
          break;
        }
      }
    }
  }
  updated();
}

void KFreeObjectManager::paint(QPainter* p)
{
  for (int idx = -1; auto& obj: objects)
  {
    idx++;
    PaintMode mode = PaintMode::Normal;
    if (idx == edited_object_idx)
      mode = PaintMode::Edited;
    else if (selected_guids.contains(obj.getGuid()))
      mode = PaintMode::Selected;
    paintObject(p, obj, mode);
  }
}

void KFreeObjectManager::acceptObject()
{
  selected_guids.clear();
  if (edited_object_idx >= 0)
  {
    auto& obj = objects[edited_object_idx];
    if (obj.getGuid().isNull())
      obj.setGuid(QUuid::createUuid());
    QString file_name = getObjectPath(obj.getGuid().toString());
    obj.save(file_name);
    saved(file_name);
    is_creating_new_object = false;
    edited_object_idx      = -1;
    pack.addObject(obj);
  }
  updated();
  finishEdit();
}

void KFreeObjectManager::loadFileWithoutUpdate(QString path)
{
  KFreeObject object;
  object.load(path, pixel_size_mm);
  objects.append(object);
}

void KFreeObjectManager::loadFileWithoutUpdate(QFileInfo file_info)
{
  loadFile(file_info.absoluteFilePath());
}

void KFreeObjectManager::loadFile(QString path)
{
  loadFileWithoutUpdate(path);
  updated();
}

void KFreeObjectManager::startMovingPoint(QPoint p0)
{
  if (edited_object_idx < 0)
    return;
  moving_point_idx = getSelectedObjectPointIdxAt(p0);
}

void KFreeObjectManager::stopMovingPoint()
{
  moving_point_idx = {-1, -1};
}

bool KFreeObjectManager::canScroll()
{
  return moving_point_idx == QPair{-1, -1};
}

void KFreeObjectManager::movePoint(QPoint p)
{
  if (edited_object_idx < 0 || edited_object_idx >= objects.count())
    return;

  auto& obj = objects[edited_object_idx];
  if (obj.polygons.isEmpty())
    obj = objects[edited_object_idx];

  if (moving_point_idx.first < 0 ||
      moving_point_idx.first >= obj.polygons.count())
    return;

  auto& polygon = obj.polygons[moving_point_idx.first];

  if (moving_point_idx.second < 0 ||
      moving_point_idx.second >= polygon.count())
    return;

  polygon[moving_point_idx.second] = scr2deg(p);
  updated();
}

QPair<int, int>
KFreeObjectManager::getSelectedObjectPointIdxAt(QPoint p0)
{
  if (edited_object_idx < 0)
    return {-1, -1};
  auto& obj           = objects[edited_object_idx];
  auto  proximity_pix = proximity_mm / pixel_size_mm;
  for (int polygon_idx = -1; auto polygon: obj.polygons)
  {
    polygon_idx++;
    QPolygon polygon_pix;
    for (int point_idx = -1; auto p: polygon)
    {
      point_idx++;
      auto pix = deg2scr(p);
      auto d   = kmath::getDistance(pix, p0);
      if (d < proximity_pix)
        return {polygon_idx, point_idx};
    }
  }
  return {-1, -1};
}

int KFreeObjectManager::getObjectIdxAt(QPoint p0)
{
  using namespace kmath;
  auto proximity_pix = proximity_mm / pixel_size_mm;
  for (int idx = -1; auto obj: objects)
  {
    idx++;
    if (obj.cl.type == KClass::Point)
    {
      auto point_pos = deg2pix(obj.polygons.first().first());
      if ((point_pos - p0).manhattanLength() < proximity_pix)
        return idx;
    }
    if (obj.cl.type == KClass::Line)
    {
      for (auto polygon: obj.polygons)
      {
        QPolygon polygon_pix;
        for (auto p: polygon)
          polygon_pix.append(deg2pix(p));
        if (isNearPolyline(p0, polygon_pix, proximity_pix))
          return idx;
      }
    }
    if (obj.cl.type == KClass::Polygon)
    {
      for (auto polygon: obj.polygons)
      {
        QPolygon polygon_pix;
        for (auto p: polygon)
          polygon_pix.append(deg2pix(p));
        if (polygon_pix.containsPoint(p0, Qt::OddEvenFill))
          return idx;
      }
    }
  }
  return -1;
}

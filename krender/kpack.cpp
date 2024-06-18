#include "math.h"
#include "kpack.h"
#include "kserialize.h"
#include "klocker.h"
#include <QDebug>
#include <QElapsedTimer>
#include <QDateTime>
#include <QRegularExpression>

void KPack::clear()
{
  if (main.status != KTile::Loaded)
    return;
  if (main.status == KTile::Loading)
    return;

  main.clear();
  tiles.clear();
  classes.clear();
  main.status = KTile::Null;
}

qint64 KPack::count()
{
  qint64 total_count = main.count();
  for (auto t: tiles)
    total_count += t.count();
  return total_count;
}

void KPack::save(QString path) const
{
  using namespace KSerialize;

  QFile f(path);
  if (!f.open(QIODevice::WriteOnly))
  {
    qDebug() << "write error:" << path;
    return;
  }

  write(&f, QString("kpack"));
  write(&f, frame);
  char has_borders = (borders.count() > 0);
  write(&f, has_borders);

  if (has_borders)
  {
    QByteArray ba;
    write(ba, borders.count());
    for (auto border: borders)
      border.save(ba, border_coor_precision_coef);
    ba = qCompress(ba, 9);
    write(&f, ba.count());
    f.write(ba.data(), ba.count());
  }

  write(&f, main_mip);
  write(&f, tile_mip);
  write(&f, classes.count());
  for (auto cl: classes)
    cl.save(&f);

  QByteArray ba;
  ba = qCompress(ba, 9);
  write(&f, ba.count());
  f.write(ba.data(), ba.count());

  write(&f, main.count());
  ba.clear();

  for (auto& obj: main)
    obj.save(classes, ba);
  ba = qCompress(ba, 9);
  write(&f, ba.count());
  f.write(ba.data(), ba.count());
  write(&f, tiles.count());
  QList<qint64> small_part_pos_list;

  for (int part_idx = 0; auto& tile: tiles)
  {
    small_part_pos_list.append(f.pos());
    if (tile.count() > 0)
    {
      write(&f, tile.count());
      ba.clear();
      for (auto& obj: tile)
        obj.save(classes, ba);
      ba = qCompress(ba, 9);
      write(&f, ba.count());
      f.write(ba.data(), ba.count());
    }
    else
      write(&f, 0);
    part_idx++;
  }
  auto small_idx_start_pos = f.pos();
  for (auto& pos: small_part_pos_list)
    write(&f, pos);
  write(&f, small_idx_start_pos);
}

void KPack::loadMain(QString path, bool load_objects,
                     double pixel_size_mm)
{
  if (main.status == KTile::Loading)
    return;
  QElapsedTimer t;
  t.start();

  using namespace KSerialize;
  QFile f(path);
  if (!f.open(QIODevice::ReadOnly))
  {
    qDebug() << "read error:" << path;
    return;
  }

  if (main.status != KTile::Null)
    return;

  QString format_id;
  read(&f, format_id);
  read(&f, frame);

  char has_borders = false;
  read(&f, has_borders);
  if (has_borders)
  {
    QByteArray ba;
    int        ba_count = 0;
    read(&f, ba_count);
    ba.resize(ba_count);
    f.read(ba.data(), ba_count);
    ba      = qUncompress(ba);
    int pos = 0;
    int borders_count;
    read(ba, pos, borders_count);
    borders.resize(borders_count);
    for (auto& border: borders)
      border.load(ba, pos, border_coor_precision_coef);
    borders_m.clear();
    for (auto border: borders)
    {
      QPolygonF border_m = border.toPolygonM();
      if (!border_m.isEmpty())
        borders_m.append(border_m);
    }
  }

  read(&f, main_mip);
  read(&f, tile_mip);

  if (!load_objects)
    return;

  qDebug() << "loading main from" << path;
  main.status = KTile::Loading;
  int class_count;
  read(&f, class_count);
  qDebug() << "class_count" << class_count;
  for (int i = 0; i < class_count; i++)
  {
    KClass cl;
    cl.load(&f, pixel_size_mm);
    classes.append(cl);
  }

  int ba_count = 0;
  read(&f, ba_count);
  QByteArray ba;
  ba.resize(ba_count);
  f.read(ba.data(), ba_count);
  ba = qUncompress(ba);

  int pos = 0;

  int big_obj_count;
  read(&f, big_obj_count);
  main.resize(big_obj_count);

  ba.clear();
  ba_count = 0;
  read(&f, ba_count);
  ba.resize(ba_count);
  f.read(ba.data(), ba_count);
  ba  = qUncompress(ba);
  pos = 0;
  for (auto& obj: main)
    obj.load(classes, pos, ba);

  int small_count;
  read(&f, small_count);
  tiles.resize(small_count);
}

void KPack::loadAll(QString path, double pixel_size_mm)
{
  loadMain(path, true, pixel_size_mm);
  for (int i = 0; i < tiles.count(); i++)
    loadTile(path, i);
}

void KPack::loadTile(QString path, int tile_idx)
{
  if (main.status != KTile::Loaded)
    return;
  if (tiles[tile_idx].status == KTile::Loading)
    return;

  qDebug() << "loading tile" << tile_idx << "from" << path;
  QElapsedTimer t;
  t.start();
  using namespace KSerialize;
  QFile f(path);
  if (!f.open(QIODevice::ReadOnly))
  {
    qDebug() << "read error:" << path;
    return;
  }

  qint64 small_idx_start_pos;
  f.seek(f.size() - sizeof(qint64));
  read(&f, small_idx_start_pos);

  int calc_small_part_count =
      (f.size() - sizeof(qint64) - small_idx_start_pos) /
      sizeof(qint64);
  if (calc_small_part_count != tiles.count())
    return;

  if (tile_idx > tiles.count() - 1)
    return;

  f.seek(small_idx_start_pos + tile_idx * sizeof(qint64));
  qint64 part_pos;
  read(&f, part_pos);
  f.seek(part_pos);

  int part_obj_count;
  read(&f, part_obj_count);

  if (part_obj_count == 0)
    return;

  tiles[tile_idx].resize(part_obj_count);

  int ba_count = 0;
  read(&f, ba_count);
  QByteArray ba;
  ba.resize(ba_count);
  f.read(ba.data(), ba_count);
  ba = qUncompress(ba);

  tiles[tile_idx].status = KTile::Loading;
  int pos                = 0;
  for (auto& obj: tiles[tile_idx])
    obj.load(classes, pos, ba);
}

void KPack::addObject(KFreeObject free_obj)
{
  if (frame.isNull())
    frame = free_obj.polygons.first().getFrame();
  else
    frame =
        free_obj.frame.united(free_obj.polygons.first().getFrame());

  auto map_size_m     = frame.getSizeMeters();
  auto map_top_left_m = frame.top_left.toMeters();

  KObject obj = free_obj;

  obj.class_idx = -1;
  for (auto cl: classes)
  {
    obj.class_idx++;
    if (cl.id == free_obj.cl.id)
      break;
  }

  if (obj.class_idx == classes.count() - 1)
    classes.append(free_obj.cl);

  int tile_side_num = sqrt(tiles.count());
  if (free_obj.cl.max_mip == 0 || free_obj.cl.max_mip > tile_mip)
    main.append(obj);
  else
  {
    auto   obj_top_left_m = obj.frame.top_left.toMeters();
    double shift_x_m      = obj_top_left_m.x() - map_top_left_m.x();
    int    part_idx_x =
        1.0 * shift_x_m / map_size_m.width() * tile_side_num;
    int shift_y = obj_top_left_m.y() - map_top_left_m.y();
    int part_idx_y =
        1.0 * shift_y / map_size_m.height() * tile_side_num;
    int tile_idx = part_idx_y * tile_side_num + part_idx_x;
    tiles[tile_idx].append(obj);
  }
}

QVector<KFreeObject> KPack::getObjects()
{
  QVector<KFreeObject> free_objects;
  for (auto src_obj: main)
  {
    KFreeObject free_obj = src_obj;
    free_obj.cl          = classes[src_obj.class_idx];
    free_objects.append(free_obj);
  }
  return free_objects;
}

#include "kclassmanager.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMetaEnum>

void KClassManager::loadClasses(QString path, QString images_dir)
{
  QFile         f(path);
  QSet<QString> id_set;
  if (f.open(QIODevice::ReadOnly))
  {
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(f.readAll(), &error);
    f.close();
    if (doc.isNull())
      return;

    if (doc.isArray())
    {
      auto ar = doc.array();
      for (auto json_val: ar)
      {
        auto obj = json_val.toObject();
        if (obj.isEmpty())
          continue;
        auto _main_mip = obj.value("main_mip").toDouble();
        if (_main_mip > 0)
        {
          main_mip = _main_mip;
          continue;
        }
        auto _tile_mip = obj.value("tile_mip").toDouble();
        if (_tile_mip > 0)
        {
          tile_mip = _tile_mip;
          continue;
        }
        auto _coor_precision_coef =
            obj.value("default_coor_precision_coef").toInt();
        if (_coor_precision_coef > 0)
        {
          default_coor_precision_coef = _coor_precision_coef;
          continue;
        }

        KClass cl;

        cl.id = obj.value("id").toString();
        if (id_set.contains(cl.id))
        {
          error_str = cl.id + " has been already defined!";
          return;
        }
        id_set.insert(cl.id);

        auto type_str = obj.value("type").toString();
        if (!type_str.isEmpty())
        {
          bool ok = false;
          cl.type = static_cast<KClass::Type>(
              QMetaEnum::fromType<KClass::Type>().keyToValue(
                  type_str.toUtf8(), &ok));
          if (!ok)
          {
            error_str = type_str + "is an unrecognized type!";
            return;
          }
        }

        cl.style       = KClass::Solid;
        auto style_str = obj.value("style").toString();
        if (!style_str.isEmpty())
        {
          bool ok  = false;
          cl.style = static_cast<KClass::Style>(
              QMetaEnum::fromType<KClass::Style>().keyToValue(
                  style_str.toUtf8(), &ok));
          if (!ok)
          {
            error_str = type_str + "is an unrecognized style!";
            return;
          }
        }

        cl.layer    = obj.value("layer").toInt();
        cl.width_mm = obj.value("width_mm").toDouble();

        auto l = obj.value("pen").toArray().toVariantList();
        if (l.count() >= 3)
          cl.pen = QColor{l.at(0).toInt(), l.at(1).toInt(),
                          l.at(2).toInt()};
        if (l.count() == 4)
          cl.pen.setAlpha(l.at(3).toInt());
        else
          cl.pen.setAlpha(255);

        l = obj.value("brush").toArray().toVariantList();
        if (l.count() >= 3)
          cl.brush = QColor{l.at(0).toInt(), l.at(1).toInt(),
                            l.at(2).toInt()};
        if (l.count() == 4)
          cl.brush.setAlpha(l.at(3).toInt());
        else
          cl.brush.setAlpha(255);

        l = obj.value("tcolor").toArray().toVariantList();
        if (l.count() >= 3)
          cl.tcolor = QColor{l.at(0).toInt(), l.at(1).toInt(),
                             l.at(2).toInt()};
        if (l.count() == 4)
          cl.tcolor.setAlpha(l.at(3).toInt());
        else
          cl.tcolor.setAlpha(255);

        cl.min_mip      = obj.value("min_mip").toDouble();
        cl.max_mip      = obj.value("max_mip").toDouble();
        auto image_name = obj.value("image").toString();
        if (!image_name.isEmpty())
        {
          auto image_path = images_dir + "/" + image_name;
          auto image      = QImage(image_path);
          if (image.isNull())
            qDebug() << "error opening" << image_path;
          else
            cl.image = image.scaledToWidth(cl.getWidthPix(),
                                           Qt::SmoothTransformation);
        }

        cl.coor_precision_coef =
            obj.value("coor_precision_coef").toInt();
        if (cl.coor_precision_coef == 0)
          cl.coor_precision_coef = default_coor_precision_coef;

        classes.append(cl);
      }
    }
  }
}

KClassManager::KClassManager(QString _images_dir)
{
  images_dir = _images_dir;
}

int KClassManager::getClassIdxById(QString id)
{
  for (int i = -1; auto cl: classes)
  {
    i++;
    if (cl.id == id)
      return i;
  }
  return -1;
}

KClass KClassManager::getClassById(QString id)
{
  auto idx = getClassIdxById(id);
  if (idx >= 0)
    return classes.at(idx);
  else
    return KClass();
}

QVector<KClass> KClassManager::getClasses()
{
  return classes;
}

KClassImageList KClassManager::getClassImageList()
{
  QVector<KClassImage> ret;
  for (auto cl: classes)
    ret.append({cl.id, cl.image});
  return ret;
}

void KClassManager::setMainMip(double v)
{
  main_mip = v;
}

double KClassManager::getMainMip()
{
  return main_mip;
}

void KClassManager::setTileMip(double v)
{
  tile_mip = v;
}

double KClassManager::getTileMip()
{
  return tile_mip;
}

void KClassManager::setDefaultCoorPrecisionCoef(double v)
{
  default_coor_precision_coef = v;
}

double KClassManager::getDefaultCoorPrecisionCoef()
{
  return default_coor_precision_coef;
}

QString KClassManager::getErrorStr()
{
  return error_str;
}

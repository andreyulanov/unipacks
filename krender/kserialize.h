#ifndef KSERIALIZE_H
#define KSERIALIZE_H

#include <type_traits>
#include <QFile>
#include <QMap>
#include <QImage>
#include <QPen>

namespace KSerialize
{
template<class T>
inline void write(QFile* f, const T& v)
{
  f->write((char*)&v, sizeof(v));
}

template<class T>
inline void read(QFile* f, T& v)
{
  f->read((char*)&v, sizeof(v));
}

inline void write(QFile* f, QByteArray ba)
{
  uchar n = ba.count();
  write(f, n);
  if (n > 0)
    f->write((char*)ba.data(), n);
}

inline void write(QFile* f, QString str)
{
  QByteArray ba = str.toUtf8();
  uchar      n  = ba.count();
  write(f, n);
  if (n > 0)
    f->write((char*)ba.data(), n);
}

inline void write(QFile* f, QImage image)
{
  int n = image.sizeInBytes();
  write(f, n);
  if (n > 0)
    image.save(f, "PNG");
}

inline void write(QFile* f, QColor c)
{
  write(f, (uchar)c.red());
  write(f, (uchar)c.green());
  write(f, (uchar)c.blue());
  write(f, (uchar)c.alpha());
}

inline void write(QFile* f, QPen pen)
{
  write(f, pen.color());
  uchar style = static_cast<uchar>(pen.style());
  write(f, style);
}

inline void write(QFile* f, QBrush brush)
{
  write(f, brush.color());
  uchar style = static_cast<uchar>(brush.style());
  write(f, style);
}

inline void read(QFile* f, QString& str)
{
  uchar n;
  read(f, n);
  if (n > 0)
  {
    QByteArray ba = f->read(n);
    str           = QString::fromUtf8(ba);
  }
}

inline void read(QFile* f, QByteArray& ba)
{
  uchar n;
  read(f, n);
  if (n > 0)
    ba = f->read(n);
}

inline void read(QFile* f, QColor& c)
{
  uchar red, green, blue, alpha;
  read(f, red);
  read(f, green);
  read(f, blue);
  read(f, alpha);
  c = QColor(red, green, blue, alpha);
}

inline void read(QFile* f, QPen& pen)
{
  QColor c;
  read(f, c);
  uchar style;
  read(f, style);
  pen.setStyle(static_cast<Qt::PenStyle>(style));
}

inline void read(QFile* f, QBrush& brush)
{
  QColor c;
  read(f, c);
  uchar style;
  read(f, style);
  brush.setStyle(static_cast<Qt::BrushStyle>(style));
}

inline void read(QFile* f, QImage& image)
{
  int n;
  read(f, n);
  if (n > 0)
    image.load(f, "PNG");
}

template<class T>
inline void write(QByteArray& ba, const T& v)
{
  int s = sizeof(v);
  ba.append((char*)&v, s);
}

template<class T>
inline void read(const QByteArray& ba, int& pos, T& v)
{
  int s = sizeof(v);
  memcpy(&v, &ba.data()[pos], s);
  pos += s;
}

inline void write(QByteArray& ba, QString str)
{
  QByteArray ba_str = str.toUtf8();
  int        n      = ba_str.count();
  write(ba, n);
  ba.append(ba_str, n);
}

inline void write(QByteArray& ba, QByteArray data)
{
  int n = data.count();
  write(ba, n);
  ba.append(data, n);
}

inline void read(const QByteArray& ba, int& pos, QString& str)
{
  int n;
  read(ba, pos, n);
  QByteArray ba_str;
  ba_str.resize(n);
  memcpy(ba_str.data(), &ba.data()[pos], n);
  pos += n;
  str = QString::fromUtf8(ba_str);
}

inline void read(const QByteArray& ba, int& pos, QByteArray& data)
{
  int n;
  read(ba, pos, n);
  data.resize(n);
  memcpy(data.data(), &ba.data()[pos], n);
  pos += n;
}

template<class Value>
inline void write(QByteArray& ba, const QVector<Value>& values)
{
  write(ba, values.count());
  for (auto v: values)
    write(ba, v);
}

inline void write(QByteArray& ba, const QStringList& str_list)
{
  write(ba, str_list.count());
  for (auto v: str_list)
    write(ba, v);
}

template<class Value>
inline void read(const QByteArray& ba, int& pos,
                 QVector<Value>& values)
{
  int n;
  read(ba, pos, n);
  for (int i = 0; i < n; i++)
  {
    Value v;
    read(ba, pos, v);
    values.append(v);
  }
}

inline void read(const QByteArray& ba, int& pos,
                 QStringList& str_list)
{
  int n;
  read(ba, pos, n);
  for (int i = 0; i < n; i++)
  {
    QString v;
    read(ba, pos, v);
    str_list.append(v);
  }
}

template<class Key, class Value>
inline void write(QByteArray& ba, const QMap<Key, Value>& map)
{
  QMapIterator<Key, Value> i(map);
  write(ba, map.count());
  while (i.hasNext())
  {
    i.next();
    write(ba, i.key());
    write(ba, i.value());
  }
}

template<class Key, class Value>
inline void read(const QByteArray& ba, int& pos,
                 QMap<Key, Value>& map)
{
  int count;
  read(ba, pos, count);
  map.clear();
  for (int i = 0; i < count; i++)
  {
    Key key;
    read(ba, pos, key);
    Value value;
    read(ba, pos, value);
    map.insert(key, value);
  }
}

template<class Key, class Value>
inline void write(QFile* f, const QMap<Key, Value>& map)
{
  QMapIterator<Key, Value> i(map);
  write(f, map.count());
  while (i.hasNext())
  {
    i.next();
    write(f, i.key());
    write(f, i.value());
  }
}

template<class Key, class Value>
inline void read(QFile* f, QMap<Key, Value>& map)
{
  int count;
  read(f, count);
  map.clear();
  for (int i = 0; i < count; i++)
  {
    Key key;
    read(f, key);
    Value value;
    read(f, value);
    map.insert(key, value);
  }
}

}
#endif  // KSERIALIZE_H

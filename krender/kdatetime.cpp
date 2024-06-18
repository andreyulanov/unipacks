#include "kdatetime.h"

#include <QStringList>
#include <math.h>

KDateTime::KDateTime()
{
  year  = 0;
  month = 0;
  day   = 0;
  hour  = 0;
  min   = 0;
  sec   = 0;
  tz    = 0;
}

static QDateTime toQDateTime(KDateTime dt)
{
  QDateTime qdt(QDate(dt.getYear(), dt.getMonth(), dt.getDay()),
                QTime(dt.getHour(), dt.getMin(), dt.getSec()));
  qdt.setOffsetFromUtc(0);
  return qdt;
}

static KDateTime fromQDateTime(QDateTime dt, double time_zone)
{
  return KDateTime(dt.date().year(), dt.date().month(),
                   dt.date().day(), dt.time().hour(),
                   dt.time().minute(), dt.time().second(), time_zone);
}

KDateTime::KDateTime(int _year, int _month, int _day, int _hour,
                     int _min, int _sec, double _tz)
{
  year  = _year;
  month = _month;
  day   = _day;
  hour  = _hour;
  min   = _min;
  sec   = _sec;
  setTimeZone(_tz);
}

KDateTime::KDateTime(QDateTime qdt)
{
  year  = qdt.date().year();
  month = qdt.date().month();
  day   = qdt.date().day();
  hour  = qdt.time().hour();
  min   = qdt.time().minute();
  sec   = qdt.time().second();
  setTimeZone(1.0 * qdt.offsetFromUtc() / 3600);
}

int KDateTime::getYear() const
{
  return year;
}

int KDateTime::getMonth() const
{
  return month;
}

int KDateTime::getDay() const
{
  return day;
}

int KDateTime::getHour() const
{
  return hour;
}

int KDateTime::getMin() const
{
  return min;
}

int KDateTime::getSec() const
{
  return sec;
}

double KDateTime::getTimeZone() const
{
  return (1.0 * int(tz)) / time_zone_coef;
}

void KDateTime::setTimeZone(double v)
{
  tz = int(v * time_zone_coef);
}

bool KDateTime::isEqual(const KDateTime& v) const
{
  return year == v.year && month == v.month && day == v.day &&
         hour == v.hour && min == v.min && sec == v.sec && tz == v.tz;
}

QString KDateTime::toString(QString format) const
{
  if (format.isEmpty())
  {
    QString str = toQDateTime(*this).toString("yyyy-MM-ddThh:mm:ss");
    return str += QString().asprintf("Z%02.1f", getTimeZone());
  }
  else
    return toQDateTime(*this).toString(format);
}

KDateTime KDateTime::fromString(QString str)
{
  KDateTime dt;
  dt.year  = str.mid(0, 4).toDouble();
  dt.month = str.mid(5, 2).toDouble();
  dt.day   = str.mid(8, 2).toDouble();
  dt.hour  = str.mid(11, 2).toDouble();
  dt.min   = str.mid(14, 2).toDouble();
  dt.sec   = str.mid(17, 2).toDouble();
  dt.setTimeZone(str.mid(20, 4).toDouble());
  return dt;
}

bool KDateTime::isValid() const
{
  return (bool)year;
}

int KDateTime::secsTo(KDateTime v) const
{
  QDateTime dt1 = toQDateTime(*this);
  dt1           = dt1.addSecs(-getTimeZone() * 3600);
  QDateTime dt2 = toQDateTime(v);
  dt2           = dt2.addSecs(-v.getTimeZone() * 3600);
  return dt1.secsTo(dt2);
}

int KDateTime::secsToWithoutTZ(KDateTime v) const
{
  QDateTime dt1 = toQDateTime(*this);
  QDateTime dt2 = toQDateTime(v);
  return dt1.secsTo(dt2);
}

KDateTime KDateTime::addDays(int days)
{
  QDateTime dt1 = toQDateTime(*this);
  QDateTime dt2 = dt1.addDays(days);
  return fromQDateTime(dt2, getTimeZone());
}

KDateTime KDateTime::addSecs(int secs)
{
  QDateTime dt1 = toQDateTime(*this);
  QDateTime dt2 = dt1.addSecs(secs);
  return fromQDateTime(dt2, getTimeZone());
}

QTime KDateTime::str2time(QString str)
{
  QStringList str_list = str.split(':');
  if (str_list.count() < 3)
    return QTime();
  int hms[3];
  for (int i = 0; i < 3; i++)
    hms[i] = str_list[i].toInt();
  QTime t(hms[0], hms[1], hms[2]);
  return t;
}

QDate KDateTime::str2date(QString str)
{
  QStringList str_list = str.split('.');
  if (str_list.count() < 3)
    return QDate();
  int ymd[3];
  for (int i = 0; i < 3; i++)
    ymd[i] = str_list[i].toInt();
  QDate d(ymd[2], ymd[1], ymd[0]);
  return d;
}

int KDateTime::rus2sec(QString str)
{
  QStringList str_list = str.split(' ');
  if (str_list.count() == 2)
  {
    int     value = str_list.at(0).toInt();
    QString units = str_list.at(1).toLower();
    if (units == "мин")
      value *= 60;
    if (units == "час" || units == "часа" || units == "часов")
      value *= 3600;
    return value;
  }
  else
    return 0;
}

QString KDateTime::sec2str(int t)
{
  int h = t / 3600;
  int m = (t - h * 3600) / 60;
  int s = t - h * 3600 - m * 60;
  if (h > 99)
    return QString().asprintf("--:--:--");
  else
    return QString().asprintf("%02d:%02d:%02d", h, m, s);
}

QString KDateTime::timezone2str(double t)
{
  int     h = (int)t;
  QString str;

  if (t > 0)
    str = "+";
  else if (t < 0)
    str = "-";
  str += QString().asprintf("%02d:%02d", abs(h),
                            (int)(fabs(t - h) * 60));
  return str;
}

double KDateTime::str2timezone(QString str)
{
  double sign = 1;
  if (str.at(0) == '-')
  {
    sign = -1;
    str.remove(0, 1);
  }
  QStringList str_list = str.split(':');

  if (str_list.count() == 2)
    return sign * (str_list.at(0).toInt() +
                   1.0 * str_list.at(1).toInt() / 60);
  else
    return 0;
}

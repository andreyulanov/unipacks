#include "qgeotilefetcherbingmaps.h"
#include "qgeomapreplybingmaps.h"
#include "qgeotiledmapbingmaps.h"
#include "qgeotiledmappingmanagerenginebingmaps.h"
#include <QtLocation/private/qgeotilespec_p.h>

#include <QDebug>
#include <QSize>
#include <QDir>
#include <QUrl>
#include <QUrlQuery>
#include <QTime>
#include <QNetworkProxy>
#include "krender/krender.h"

#include <math.h>

#include <map>

QT_BEGIN_NAMESPACE

namespace
{

int _getServerNum(int x, int y, int max)
{
  return (x + 2 * y) % max;
}

QString _tileXYToQuadKey(int tileX, int tileY, int levelOfDetail)
{
  QString quadKey;
  for (int i = levelOfDetail; i > 0; i--)
  {
    char digit = '0';
    int  mask  = 1 << (i - 1);
    if ((tileX & mask) != 0)
    {
      digit++;
    }
    if ((tileY & mask) != 0)
    {
      digit++;
      digit++;
    }
    quadKey.append(digit);
  }
  return quadKey;
}
}

QGeoTileFetcherBingmaps::QGeoTileFetcherBingmaps(
    const QVariantMap&                     parameters,
    QGeoTiledMappingManagerEngineBingmaps* engine,
    const QSize&                           tileSize):
    QGeoTileFetcher(engine),
    m_networkManager(new QNetworkAccessManager(this)),
    m_engineBingmaps(engine), m_tileSize(tileSize)
{
  qDebug() << Q_FUNC_INFO << "tileSize" << tileSize;

  if (parameters.contains(QStringLiteral("bingmaps.useragent")))
    _userAgent =
        parameters.value(QStringLiteral("bingmaps.useragent"))
            .toString()
            .toLatin1();
  else
    _userAgent = "Mozilla/5.0 (X11; Linux i586; rv:31.0) "
                 "Gecko/20100101 Firefox/31.0";

  QStringList langs = QLocale::system().uiLanguages();
  if (langs.length() > 0)
  {
    _language = langs[0];
  }

  // Google version strings
  _versionBingMaps = "563";

  netRequest.setRawHeader("Referrer", "https://www.bing.com/maps/");
  netRequest.setRawHeader("Accept", "*/*");
  netRequest.setRawHeader("User-Agent", _userAgent);
}

QGeoTileFetcherBingmaps::~QGeoTileFetcherBingmaps()
{
}

QGeoTiledMapReply*
QGeoTileFetcherBingmaps::getTileImage(const QGeoTileSpec& spec)
{
  QString surl =
      _getURL(spec.mapId(), spec.x(), spec.y(), spec.zoom());
  QUrl url(surl);

  netRequest.setUrl(url);

  QNetworkReply* netReply = m_networkManager->get(netRequest);

  qDebug() << Q_FUNC_INFO << "m_tileSize" << m_tileSize;
  qDebug() << Q_FUNC_INFO << "spec" << spec;

  KRender::Settings s;
  s.pixel_size_mm = 0.1;
  s.window_size   = m_tileSize;
  KRender* render = new KRender(s);
  render->addPack("/home/user/unipacks/data/packs/world.kpack", true);
  render->addPack(
      "/home/user/unipacks/data/packs/ru-spe,ru-len.kpack", false);
  render->requestTile(m_tileSize.width(), spec.x(), spec.y(),
                      spec.zoom());
  QGeoTiledMapReply* mapReply =
      new QGeoMapReplyBingmaps(netReply, render, spec);

  return mapReply;
}

QString QGeoTileFetcherBingmaps::_getURL(int type, int x, int y,
                                         int zoom)
{
  switch (type)
  {
  case 0:
  case 1:
  {
    const QString key = _tileXYToQuadKey(x, y, zoom);
    return QString("http://ecn.t%1.tiles.virtualearth.net/tiles/"
                   "r%2.png?g=%3&mkt=%4")
        .arg(_getServerNum(x, y, 4))
        .arg(key)
        .arg(_versionBingMaps)
        .arg(_language);
  }
  break;
  case 2:
  {
    const QString key = _tileXYToQuadKey(x, y, zoom);
    return QString("http://ecn.t%1.tiles.virtualearth.net/tiles/"
                   "a%2.jpeg?g=%3&mkt=%4")
        .arg(_getServerNum(x, y, 4))
        .arg(key)
        .arg(_versionBingMaps)
        .arg(_language);
  }
  break;
  case 3:
  {
    const QString key = _tileXYToQuadKey(x, y, zoom);
    return QString("http://ecn.t%1.tiles.virtualearth.net/tiles/"
                   "h%2.jpeg?g=%3&mkt=%4")
        .arg(_getServerNum(x, y, 4))
        .arg(key)
        .arg(_versionBingMaps)
        .arg(_language);
  }
  }
  return "";
}

void QGeoTileFetcherBingmaps::_networkReplyError(
    QNetworkReply::NetworkError error)
{
  qWarning() << "Could not connect to Bing maps. Error:" << error;
  if (_bingReply)
  {
    _bingReply->deleteLater();
    _bingReply = NULL;
  }
}

void QGeoTileFetcherBingmaps::_replyDestroyed()
{
  _bingReply = NULL;
}

QT_END_NAMESPACE

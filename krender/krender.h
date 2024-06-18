#ifndef KRENDER_H
#define KRENDER_H

#include "krenderpack.h"
#include <QReadWriteLock>
#include <QThread>
#include <QSet>
#include <QMutex>

class KRender: public QThread
{
  friend class KRenderThread;

public:
  struct Settings
  {
    double pixel_size_mm = 0.1;
    QSize  window_size;
    QColor background_color      = QColor(150, 210, 240);
    double max_loaded_maps_count = 3;
  };
  struct Tile
  {
    int x, y, z;
  };

private:
  struct DrawTextEntry
  {
    QString       text;
    const KClass* cl;
    QRect         rect;
    QRect         actual_rect;
    Qt::Alignment alignment;
  };

  struct NameHolder
  {
    int            length_pix  = 0;
    int            start_idx   = 0;
    int            end_idx     = 0;
    int            point_count = 0;
    double         angle_deg   = 0;
    QPoint         mid_point;
    const KObject* obj = nullptr;
    QColor         tcolor;
    void           fix(const KPack* pack, const KObject* obj,
                       const QPoint& start, const QPoint& end);
  };

  struct PointName
  {
    QRect         rect;
    QStringList   str_list;
    const KClass* cl;
  };

  Q_OBJECT

  double render_window_size_coef      = 0;
  QColor ocean_color                  = QColor(150, 210, 240);
  QColor land_color                   = QColor(250, 246, 230);
  int    update_interval_ms           = 0;
  int    max_loaded_maps_count        = 0;
  double max_object_size_with_name_mm = 20.0;

  QPointF       render_center_m;
  double        mip        = 1;
  double        render_mip = 1;
  QPixmap       main_pixmap;
  QPixmap       render_pixmap;
  QElapsedTimer time_since_last_repaint;
  bool          rendering_enabled      = false;
  bool          loading_enabled        = true;
  bool          getting_pixmap_enabled = false;

  QPointF               center_m;
  QSize                 pixmap_size   = {100, 100};
  double                pixel_size_mm = 0.1;
  KRenderPackCollection packs;
  QFont                 font;

  QVector<PointName>     point_names[KRenderPack::render_count];
  QVector<DrawTextEntry> draw_text_array[KRenderPack::render_count];
  QVector<NameHolder>    name_holder_array[KRenderPack::render_count];
  QVector<QRect>         text_rect_array;
  QSizeF                 size_m;
  QPointF                render_top_left_m;
  QRectF                 render_frame_m;
  QElapsedTimer          yield_timer;

  Tile          curr_tile;
  QVector<Tile> pending_tiles;

  void run();
  void start() = delete;
  void insertPack(int idx, QString path, bool load_now);
  void renderPack(QPainter* p, const KRenderPack* pack,
                  int render_idx, int line_iter);
  void render(QPainter* p, QVector<KRenderPack*> render_packs,
              int render_idx);

  bool checkMipRange(const KPack* pack, const KObject* obj);
  bool canContinue();
  void checkYieldResult();

  bool paintObject(QPainter* p, const KRenderPack* map,
                   const KObject& obj, int render_idx, int line_iter);
  bool paintPointNames(QPainter* p);
  bool paintLineNames(QPainter* p);
  bool paintPolygonNames(QPainter* p);

  bool isCluttering(const QRect&);
  void paintOutlinedText(QPainter* p, const DrawTextEntry& dte);
  void addDrawTextEntry(QVector<DrawTextEntry>& draw_text_array,
                        DrawTextEntry           new_dte);

  QPolygon poly2pix(const KGeoPolygon& polygon);
  void     paintPointObject(QPainter* p, const KRenderPack& pack,
                            const KObject& obj, int render_idx);
  void     paintPolygonObject(QPainter* p, const KRenderPack& pack,
                              const KObject& obj, int render_idx);
  void     paintLineObject(QPainter* painter, const KRenderPack& pack,
                           const KObject& obj, int render_idx,
                           int line_iter);
  QRectF   getDrawRectM() const;
  bool     needToLoadPack(const KRenderPack* pack,
                          const QRectF&      draw_rect);
  void     checkLoad();
  void     checkUnload();
  void     onLoaded();

signals:
  void started(QRectF);
  void paintUserObjects(QPainter* p);
  void rendered(int ms_elapsed);
  void renderedTile(QPixmap, int x, int y, int z);

public:
  KRender(Settings);
  virtual ~KRender();
  void addPack(QString path, bool load_now);

  void         requestTile(Tile);
  void         onRenderedTile(QPixmap, int x, int y, int z);
  const KPack* getPack(int idx) const;

  void setBackgroundColor(QColor);
  void setPixelSizeMM(double);
  void setMaxLoadedMapsCount(int);

  void           setMip(double);
  double         getMip() const;
  void           setCenterM(QPointF);
  QPointF        getCenterM() const;
  QPointF        getRenderCenterM() const;
  void           setPixmapSize(QSize);
  void           setUpdateIntervalMs(int ms);
  double         getRenderWindowSizeCoef() const;
  void           setRenderWindowSizeCoef(double);
  void           paintPointName(QPainter* p, const QString& text,
                                const QColor& tcolor);
  static void    paintOutlinedText(QPainter* p, const QString& text,
                                   const QColor& tcolor);
  const QPixmap* getPixmap() const;
  const KPack*   getWorldPack() const;
  void           render();
  void           renderUserObjects();
  void           stopAndWait();
  void           enableLoading(bool);

  QPoint deg2pix(KGeoCoor) const;

  QPoint  meters2pix(QPointF m) const;
  QPointF pix2meters(QPointF pix) const;

  QPoint   deg2scr(const KGeoCoor& deg) const;
  KGeoCoor scr2deg(QPoint pix) const;

  void pan(QPoint);
  void zoom(double);
};
Q_DECLARE_METATYPE(KRender::Tile)

#endif  // KRENDER_H

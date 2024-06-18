TARGET = qtgeoservices_bingmaps
QT += location-private
CONFIG += c++2a

PLUGIN_TYPE = geoservices
PLUGIN_CLASS_NAME = QGeoServiceProviderFactoryBingMaps
load(qt_plugin)

HEADERS += \
    krender/kbase.h \
    krender/kclass.h \
    krender/kclassmanager.h \
    krender/kdatetime.h \
    krender/kfreeobjectmanager.h \
    krender/klocker.h \
    krender/kobject.h \
    krender/kpack.h \
    krender/krender.h \
    krender/krenderpack.h \
    krender/kserialize.h \
    qgeoserviceproviderpluginbingmaps.h \
    qgeomapreplybingmaps.h \
    qgeotiledmapbingmaps.h \
    qgeotiledmappingmanagerenginebingmaps.h \
    qgeotilefetcherbingmaps.h

SOURCES += \
    krender/kbase.cpp \
    krender/kclass.cpp \
    krender/kclassmanager.cpp \
    krender/kdatetime.cpp \
    krender/kfreeobjectmanager.cpp \
    krender/klocker.cpp \
    krender/kobject.cpp \
    krender/kpack.cpp \
    krender/krender.cpp \
    krender/krenderpack.cpp \
    qgeoserviceproviderpluginbingmaps.cpp \
    qgeomapreplybingmaps.cpp \
    qgeotiledmapbingmaps.cpp \
    qgeotiledmappingmanagerenginebingmaps.cpp \
    qgeotilefetcherbingmaps.cpp


OTHER_FILES += \
    bingmaps_plugin.json


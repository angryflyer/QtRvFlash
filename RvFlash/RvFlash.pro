QT       += core gui widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

INCLUDEPATH += $$PWD/include/

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

#RC_ICONS = $$PWD/Resources/image/logo.ico

SOURCES += \
    $$PWD/src/flashcfg.cpp \
    $$PWD/src/flashthread.cpp \
    $$PWD/src/console.cpp \
    $$PWD/src/flashrom.cpp \
    $$PWD/src/flashcfg.cpp \
    $$PWD/src/stdafx.cpp \
    $$PWD/main.cpp \
    $$PWD/mainwindow.cpp


HEADERS += \
    $$PWD/include/flashcfg.h \
    $$PWD/include/flashthread.h \
    $$PWD/include/console.h \
    $$PWD/include/stdafx.h \
    $$PWD/include/targetver.h \
    $$PWD/include/flashrom.h \
    $$PWD/include/flashcfg.h \
    $$PWD/include/sw_header.h \
    $$PWD/include/station_cache.h \
    $$PWD/include/station_dma.h \
    $$PWD/include/station_flash.h \
    $$PWD/include/station_orv32.h \
    $$PWD/include/station_sdio.h \
    $$PWD/include/station_slow_io.h \
    $$PWD/include/ftd2xx.h \
    $$PWD/mainwindow.h

FORMS += \
    mainwindow.ui

RESOURCES += \
    Resources/logo.qrc

LIBS += -L$$PWD/lib/ -lftd2xx

TRANSLATIONS += \
    RvFlash_zh_CN.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    Resources/image/logo.ico

RC_FILE = RvFlash.rc

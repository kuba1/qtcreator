# CONFIG += single
include(../../qtcreatorplugin.pri)

QT += gui
SOURCES += fakevimactions.cpp \
    fakevimhandler.cpp \
    fakevimplugin.cpp \
    easymotionhandler.cpp
HEADERS += fakevimactions.h \
    fakevimhandler.h \
    fakevimplugin.h \
    fakevimtr.h \
    easymotionhandler.h
FORMS += fakevimoptions.ui

equals(TEST, 1) {
    SOURCES += fakevim_test.cpp
}

RESOURCES += \
    fakevim.qrc

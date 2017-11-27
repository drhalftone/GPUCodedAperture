QT       += core gui widgets opengl

TEMPLATE  = app
DEFINES  += QT_DEPRECATED_WARNINGS

DEFINES  += CASSI

HEADERS  += laucodedapertureglfilter.h \
            laumemoryobject.h \
            lauscan.h \
            lauabstractgpsrobject.h

SOURCES  += main.cpp \
            laucodedapertureglfilter.cpp \
            laumemoryobject.cpp \
            lauscan.cpp \
            lauabstractgpsrobject.cpp

RESOURCES += cassi.qrc

CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

unix:macx {
    QMAKE_MAC_SDK   = macosx10.12
    QMAKE_CXXFLAGS += -msse2 -msse3 -mssse3 -msse4.1
    INCLUDEPATH    += /usr/local/include /usr/local/include/eigen3
    DEPENDPATH     += /usr/local/include /usr/local/include/eigen3
    LIBS           += /usr/local/lib/libtiff.5.dylib
}

unix:!macx {
    QMAKE_CXXFLAGS += -msse2 -msse3 -mssse3 -msse4.1
    INCLUDEPATH    += /usr/include /usr/include/eigen3
    DEPENDPATH     += /usr/include /usr/include/eigen3
    LIBS           += -ltiff
}

win32 {
    INCLUDEPATH += $$quote(C:/usr/include)
    DEPENDPATH  += $$quote(C:/usr/include)
    LIBS        += -L$$quote(C:/usr/lib) -llibtiff_i -lopengl32
}

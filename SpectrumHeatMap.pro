QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# FFTW libraries are in directories labeled 32 or 64. Determine which directory to find the libraries in.
contains(QT_ARCH, i386) {
    ARCHDIR = 32
} else {
    ARCHDIR = 64
}
FFTWDIR = ../fftw-dll

SOURCES += \
    fftHelper.cpp \
    hsvrgb.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    fftHelper.h \
    hsvrgb.h \
    mainwindow.h

FORMS += \
    mainwindow.ui

INCLUDEPATH += $$FFTWDIR
win32 {
    LIBS += -lws2_32
    LIBS += -L$$FFTWDIR/$$ARCHDIR -lfftw3-3
} else {
    LIBS += -L$$FFTWDIR/$$ARCHDIR -lfftw3
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

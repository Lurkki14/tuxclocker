#-------------------------------------------------
#
# Project created by QtCreator 2018-11-02T23:09:52
#
#-------------------------------------------------

QT       += core gui x11extras

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = tuxclocker
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11
CONFIG (release, debug|release) {
           DEFINES += QT_NO_DEBUG_OUTPUT
       }

#DEFINES += NVIDIA
DEFINES += AMD

SOURCES += \
        main.cpp \
        mainwindow.cpp \
    qcustomplot.cpp \
    editprofile.cpp \
    newprofile.cpp \
    plotwidget.cpp \
    nvidia.cpp \
    gputypes.cpp \
    amd.cpp

HEADERS += \
        mainwindow.h \
    qcustomplot.h \
    editprofile.h \
    newprofile.h \
    plotwidget.h \
    #nvidia.h \
    nvml.h \
    gputypes.h \
    #xlibvars.h

FORMS += \
        mainwindow.ui \
    editprofile.ui \
    newprofile.ui

INCLUDEPATH += "/usr/lib"
INCLUDEPATH += $$(INCLUDEPATH)
INCLUDEPATH += "/usr/include/libdrm"

#LIBS += -lXext -lXNVCtrl -lX11 -lnvidia-ml
LIBS += -ldrm -ldrm_amdgpu

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc


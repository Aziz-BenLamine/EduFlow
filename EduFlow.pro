QT       += core gui
QT  += sql
QT += charts
QT += multimedia multimediawidgets
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    connection.cpp \
    employe.cpp \
    main.cpp \
    mainwindow.cpp \
    statswidgetemp.cpp

HEADERS += \
    connection.h \
    employe.h \
    mainwindow.h \
    statswidgetemp.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    res/res.qrc

INCLUDEPATH += C:\opencv_contrib-4.9.0\install\include

LIBS += C:\opencv_contrib-4.9.0\bin\libopencv_core490.dll
LIBS += C:\opencv_contrib-4.9.0\bin\libopencv_highgui490.dll
LIBS += C:\opencv_contrib-4.9.0\bin\libopencv_imgproc490.dll
LIBS += C:\opencv_contrib-4.9.0\bin\libopencv_imgcodecs490.dll
LIBS += C:\opencv_contrib-4.9.0\bin\libopencv_videoio490.dll
LIBS += C:\opencv_contrib-4.9.0\bin\libopencv_video490.dll
LIBS += C:\opencv_contrib-4.9.0\bin\libopencv_objdetect490.dll
LIBS += C:\opencv_contrib-4.9.0\bin\libopencv_calib3d490.dll
LIBS += C:\opencv_contrib-4.9.0\bin\libopencv_features2d490.dll
LIBS += C:\opencv_contrib-4.9.0\bin\libopencv_flann490.dll
LIBS += C:\opencv_contrib-4.9.0\bin\libopencv_dnn490.dll
LIBS += C:\opencv_contrib-4.9.0\bin\libopencv_face490.dll

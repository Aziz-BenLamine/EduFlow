QT += core gui sql widgets charts network serialport multimedia multimediawidgets
QT += location positioning quick quickwidgets quickcontrols2
QT += texttospeech

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

SOURCES += \
    arduino.cpp \
    colis.cpp \
    connection.cpp \
    employe.cpp \
    equipements.cpp \
    etablissement.cpp \
    examen.cpp \
    main.cpp \
    mainwindow.cpp \
    qrcodegen.cpp \
    statistics_window.cpp \
    statswidgetemp.cpp

HEADERS += \
    arduino.h \
    colis.h \
    connection.h \
    employe.h \
    equipements.h \
    etablissement.h \
    examen.h \
    mainwindow.h \
    qrcodegen.hpp \
    statistics_window.h \
    statswidgetemp.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
TARGET = EduFlow

RESOURCES += \
    res/res.qrc \
    resources.qrc

INCLUDEPATH += C:\opencv_contrib-4.9.0\install\include

LIBS += C:\opencv_contrib-4.9.0\bin\libopencv_core490.dll \
        C:\opencv_contrib-4.9.0\bin\libopencv_highgui490.dll \
        C:\opencv_contrib-4.9.0\bin\libopencv_imgproc490.dll \
        C:\opencv_contrib-4.9.0\bin\libopencv_imgcodecs490.dll \
        C:\opencv_contrib-4.9.0\bin\libopencv_videoio490.dll \
        C:\opencv_contrib-4.9.0\bin\libopencv_video490.dll \
        C:\opencv_contrib-4.9.0\bin\libopencv_objdetect490.dll \
        C:\opencv_contrib-4.9.0\bin\libopencv_calib3d490.dll \
        C:\opencv_contrib-4.9.0\bin\libopencv_features2d490.dll \
        C:\opencv_contrib-4.9.0\bin\libopencv_flann490.dll \
        C:\opencv_contrib-4.9.0\bin\libopencv_dnn490.dll \
        C:\opencv_contrib-4.9.0\bin\libopencv_face490.dll

DISTFILES += \
    speech.py

contains(QT_CONFIG, opengl): QT += opengl
QT += 
INCLUDEPATH += /c/PFM_ABEv7.0.0_Win64/include
LIBS += -L /c/PFM_ABEv7.0.0_Win64/lib -lnvutility -llas -lwsock32 -lm
DEFINES += WIN32 NVWIN3X UINT32_C INT32_C
CONFIG += console
CONFIG += exceptions
QMAKE_CXXFLAGS += -fno-strict-aliasing
QMAKE_LFLAGS += 
######################################################################
# Automatically generated by qmake (2.01a) Wed Jan 22 13:45:50 2020
######################################################################

TEMPLATE = app
TARGET = las_zero
DEPENDPATH += .
INCLUDEPATH += .

# Input
HEADERS += las_zero.hpp slas.hpp version.hpp
SOURCES += las_zero.cpp slas.cpp
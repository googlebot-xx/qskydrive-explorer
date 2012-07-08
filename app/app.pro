TEMPLATE = app
QT += core gui network webkit

OBJECTS_DIR = ../.obj
MOC_DIR = ../.moc
UI_DIR = ../.ui
RCC_DIR = ../.rcc

DESTDIR = ../bin

INCLUDEPATH += ../lib/src
LIBS += -L../bin -lqlive

include(src/src.pri)

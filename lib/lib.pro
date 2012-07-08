TEMPLATE = lib
TARGET = qlive
QT += core gui network webkit

LIBS += -lqjson

OBJECTS_DIR = ../.obj
MOC_DIR = ../.moc
UI_DIR = ../.ui
RCC_DIR = ../.rcc

DESTDIR = ../bin

include(src/src.pri)


#-------------------------------------------------
#
# Project created by QtCreator 2015-08-26T20:57:17
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = rtfilt
DESTDIR = bin
OBJECTS_DIR = build
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += main.cpp

HEADERS += mdaio.h usagetracking.h
SOURCES += mdaio.cpp usagetracking.cpp

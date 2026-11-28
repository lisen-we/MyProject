QT += core gui widgets network
CONFIG += c++11
TEMPLATE = app
TARGET = LinuxChatRoomQtClient

win32-msvc*:QMAKE_CXXFLAGS += /utf-8

SOURCES += main.cpp LoginPage.cpp ClientMessage.cpp ServerMessage.cpp
HEADERS += LoginPage.h ClientMessage.h ServerMessage.h
FORMS += LoginPage.ui

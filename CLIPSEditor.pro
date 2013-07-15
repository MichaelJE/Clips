#-------------------------------------------------
#
# Project created by QtCreator 2011-06-29T14:17:21
#
#-------------------------------------------------

#CONFIG += console debug

TARGET = clipseditor
CONFIG += help

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    highlighter.cpp \
    helpviewer.cpp \
    codeeditor.cpp \
    configdialog.cpp \
    keywords.cpp \
    config.cpp \
    searchbar.cpp \
    sessions.cpp \
    snippets.cpp

HEADERS  += \
    mainwindow.h \
    highlighter.h \
    helpviewer.h \
    codeeditor.h \
    configdialog.h \
    config.h \
    keywords.h \
    searchbar.h \
    sessions.h \
    snippets.h

RESOURCES += \
    clipseditor.qrc

FORMS += \
    searchbar.ui \
    configdialog.ui \
    sessions.ui \
    snippets.ui

#QMAKE_CXXFLAGS_DEBUG += -pg
#QMAKE_LFLAGS_DEBUG += -pg


# Qt 4.7.1 bug, должна быть новая строка
# http://www.xm1math.net/texmaker/
# LatexEditor::fold()

QT_VER_CHECK = $$find(QT_VERSION, "^4\\.[0-7]\\.[0-2]")

!isEmpty(QT_VER_CHECK) {
    error("Cannot build with Qt version $${QT_VERSION} use at least 4.7.3")
}

/*===========================================================================
    CLIPSEditor, editor for CLIPS (C Language Integrated Production System)
    Copyright (C) 2012 Novikov Artem Gennadievich

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>
===========================================================================*/

#ifndef HELPVIEWER_H
#define HELPVIEWER_H

#include <QtGui>
#include <QtHelp>

#include "config.h"

class HelpBrowser : public QTextBrowser
{
    Q_OBJECT

public:
    HelpBrowser(QHelpEngine* helpEngine, QWidget* parent = 0)
        : QTextBrowser(parent), helpEngine(helpEngine) {}

    QVariant loadResource(int type, const QUrl& url)
    {
        return url.scheme() == "qthelp" ? QVariant(helpEngine->fileData(url)) :
                                          QTextBrowser::loadResource(type, url);
    }

public slots:
    void setSource(const QUrl& url)
    {
        if (url.scheme() == "http")
            QDesktopServices::openUrl(url);
        else
            QTextBrowser::setSource(url);
    }

private:
    QHelpEngine*  helpEngine;
};


class HelpViewer : public QDialog
{
    Q_OBJECT

public:
    HelpViewer(Config*);
    ~HelpViewer();

    void setIndex(const QString&);

private:
    QHelpEngine* helpEngine;
    HelpBrowser* helpBrowser;
    QTabWidget*  tabWidget;

    QFile        qhc;
    QFile        qch;
    QDir         dir;

    Config*      config;
};

#endif // MAINWINDOW_H

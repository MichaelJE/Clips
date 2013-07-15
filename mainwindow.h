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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui>

#include "config.h"
#include "codeeditor.h"
#include "helpviewer.h"
#include "configdialog.h"
#include "searchbar.h"
#include "sessions.h"
#include "snippets.h"

#define EDITOR static_cast<CodeEditor*>(tabWidget->currentWidget())

class TabWidget : public QTabWidget
{
public:
    QTabBar* tabBar() const { return QTabWidget::tabBar(); }
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(Config*, QWidget* = 0);
    ~MainWindow();

private slots:
    void newFile()        { tabWidget->setCurrentIndex(tabWidget->addTab(new CodeEditor(config), tr("NEW %1").arg(++newNumber))); }

    void openFile(QString = "");
    void openFiles(QStringList);
    void openRecentFile() { openFile((static_cast<QAction*>(sender()))->data().toString()); }

    void closeFile()
    {
        if (maybeSave())
            tabWidget->removeTab(tabWidget->currentIndex());
    }

    void closeAll();
    bool saveFile();
    bool saveFileAs();
    void saveAll();

    void loadSession(QStringList, bool);
    void insertSnippet(QString txt) { EDITOR->insertPlainText(txt); }

    void searchReplace(QString, QString, int);

    void preferences()    { configDialog->exec(); }
    void help();
    void about();

    void tabContextMenu(const QPoint&);

    void modificationChanged(bool);
    void currentChanged(int);
    void updateRecentFiles();

    void showDock();

private:
    void closeEvent(QCloseEvent*);

    bool maybeSave();
    void setCurrentFile(QString&);
    void loadFile(QString&);
    bool saveFile(QString&);

    QString currentPath();

    void addDock(QWidget*, QAction*, QString, Qt::DockWidgetArea, Qt::DockWidgetAreas = Qt::AllDockWidgetAreas);

    int             newNumber;
    QString			lastPath;

    QAction*        saveAct;
    QAction*        separatorAct;
    QList<QAction*> recentFileActs;

    TabWidget*      tabWidget;
    ConfigDialog*   configDialog;
    Sessions*       sessions;
    Snippets*       snippets;
    SearchBar*      searchBar;
    HelpViewer*     helpViewer;

    Config*         config;

    QHash<QString, QAction*> docks;
};

#endif // MAINWINDOW_H

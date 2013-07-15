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

#include "mainwindow.h"

MainWindow::MainWindow(Config* config, QWidget* parent)
    : QMainWindow(parent), config(config)
{
    tabWidget = new TabWidget;
    tabWidget->setMovable(true);
    tabWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    sessions     = new Sessions(config, tabWidget);
    snippets	 = new Snippets(config);
    searchBar    = new SearchBar(config);
    helpViewer   = new HelpViewer(config);
    configDialog = new ConfigDialog(config, this);

    QMenu* menu = menuBar()->addMenu(tr("&File"));
    connect(menu, SIGNAL(aboutToShow()),    this, SLOT(updateRecentFiles()));

    menu->addAction(tr("&New"),             this, SLOT(newFile()),     QKeySequence::New);
    menu->addAction(tr("&Open..."),         this, SLOT(openFile()),    QKeySequence::Open);

    saveAct = menu->addAction(tr("&Save"),  this, SLOT(saveFile()),    QKeySequence::Save);

    menu->addAction(tr("Save &As..."),      this, SLOT(saveFileAs()),  Qt::CTRL + Qt::ALT + Qt::Key_S);
    menu->addAction(tr("Sav&e All"),        this, SLOT(saveAll()),     Qt::CTRL + Qt::SHIFT + Qt::Key_S);
    menu->addAction(tr("C&lose All"),       this, SLOT(closeAll()),    Qt::CTRL + Qt::SHIFT + Qt::Key_C);

    QAction* action = menu->addAction(tr("Sessio&ns..."), this, SLOT(showDock()), Qt::CTRL + Qt::Key_W);
    addDock(sessions, action, tr("Sessions"), Qt::LeftDockWidgetArea);//, Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    action = menu->addAction(tr("Sni&ppets..."), this, SLOT(showDock()), Qt::CTRL + Qt::Key_P);
    addDock(snippets, action, tr("Snippets"), Qt::LeftDockWidgetArea);//, Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    menu->addSeparator();
    action = menu->addAction(tr("S&earch && Replace..."), this, SLOT(showDock()), QKeySequence::Find);
    addDock(searchBar, action, tr("Search & Replace"), Qt::BottomDockWidgetArea);//, Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    menu->addSeparator();

    separatorAct = menu->addSeparator();

    for (int i = 0; i < config->maxRecentFiles; i++) {
        QAction* action = menu->addAction("", this, SLOT(openRecentFile()));
        action->setVisible(false);

        recentFileActs << action;
    }

    menu->addSeparator();
    menu->addAction(tr("&Quit"),           this, SLOT(close()), Qt::CTRL + Qt::Key_Q);
    menu = menuBar()->addMenu(tr("&Settings"));
    menu->addAction(tr("&Preferences..."), this, SLOT(preferences()));
    menu = menuBar()->addMenu(tr("&Help"));
    menu->addAction(tr("&CLIPS v6.24"),    this, SLOT(help()),  QKeySequence::HelpContents);
    menu->addSeparator();
    menu->addAction(tr("&About..."),       this, SLOT(about()));

    currentChanged(-1); // newFile

    int index = -1;

    for (int i = 0; i < config->openFiles.count(); i++)
        if (QFileInfo(config->openFiles[i]).exists()) {

            openFile(config->openFiles[i]);

            if (config->openFiles[i] == config->lastFile)
                index = i;
        }

    setCentralWidget(tabWidget);
    setWindowTitle(PROGNAME);

    restoreGeometry(config->mainWindowGeometry);
    restoreState(config->mainWindowState);

    tabWidget->setCurrentIndex(index == -1 ? tabWidget->count() - 1 : index);
    tabWidget->currentWidget()->setFocus();
    currentChanged(0); // currentWidget

    connect(tabWidget, SIGNAL(currentChanged(int)), SLOT(currentChanged(int)));
    connect(tabWidget, SIGNAL(customContextMenuRequested(const QPoint&)), SLOT(tabContextMenu(const QPoint &)));
    connect(sessions,  SIGNAL(load(QStringList, bool)), SLOT(loadSession(QStringList, bool)));
    connect(snippets,  SIGNAL(insert(QString)),   SLOT(insertSnippet(QString)));
    connect(searchBar, SIGNAL(searchReplace(QString,QString,int)), SLOT(searchReplace(QString,QString,int)));
}

MainWindow::~MainWindow()
{
    config->mainWindowGeometry = saveGeometry();
    config->mainWindowState    = saveState();

    delete helpViewer;
}

void MainWindow::openFile(QString fileName)
{
    QStringList fileNames(fileName);

    if (fileName.isEmpty())
        fileNames = QFileDialog::getOpenFileNames(this, "", currentPath(), tr("CLIPS files (*.clp *.bat);;All types (*)"));

//    foreach (QString fileName, fileNames) {
//        if (!(tabWidget->count() == 1 && !EDITOR->document()->isModified() &&
//            tabWidget->currentWidget()->windowFilePath().isEmpty()))
//            newFile();

//        loadFile(fileName);
//    }

    openFiles(fileNames);
}

void MainWindow::openFiles(QStringList fileNames)
{
    foreach (QString fileName, fileNames) {
        if (!(tabWidget->count() == 1 && !EDITOR->document()->isModified() &&
            tabWidget->currentWidget()->windowFilePath().isEmpty()))
            newFile();

        loadFile(fileName);
    }
}

void MainWindow::closeAll()
{
    for (int i = 0; i < tabWidget->count(); i++) {
        tabWidget->setCurrentIndex(i);

        if (!maybeSave())
            return;
    }

    int i = tabWidget->count();

    while (i-- > 0)
        tabWidget->removeTab(i);
}

bool MainWindow::saveFile()
{
    QString fileName = tabWidget->currentWidget()->windowFilePath();

    if (fileName.isEmpty())
        return saveFileAs();

    return saveFile(fileName);
}

bool MainWindow::saveFileAs()
{
    QString fileName = QFileDialog::getSaveFileName(this, "", currentPath() + "/" + QFileInfo(tabWidget->currentWidget()->windowFilePath()).fileName(),
                                                    tr("CLIPS files (*.clp *.bat);;All types (*)"));
    if (fileName.isEmpty())
        return false;

    return saveFile(fileName);
}

void MainWindow::saveAll()
{
    int index = tabWidget->currentIndex();

    for (int i = 0; i < tabWidget->count(); i++) {
        tabWidget->setCurrentIndex(i);
        saveFile();
    }

    tabWidget->setCurrentIndex(index);
}

void MainWindow::loadSession(QStringList fileNames, bool leaveOpen)
{
    if (!leaveOpen)
        closeAll();

    openFiles(fileNames);
}

void MainWindow::searchReplace(QString str1, QString str2, int flags)
{
    if (flags & SearchBar::ReplaceAll) {
        for (int i = 0; i < tabWidget->count(); i++) {
            tabWidget->setCurrentIndex(i);
            EDITOR->replaceAll(str1, str2, flags);
        }
    } else if ((str2.isNull() && EDITOR->search(str1, flags) == false ||
        !str2.isNull() && EDITOR->replace(str1, str2, flags) == false) &&
        (flags & SearchBar::AllFiles)) {

        if (flags & SearchBar::Backward) {
            if (tabWidget->currentIndex() != 0) {
                tabWidget->setCurrentIndex(tabWidget->currentIndex() - 1);
                EDITOR->moveCursor(QTextCursor::End);
            }
        } else if (tabWidget->currentIndex() < tabWidget->count() - 1) {
            tabWidget->setCurrentIndex(tabWidget->currentIndex() + 1);
        }
    }
}

void MainWindow::help()
{
    helpViewer->setIndex(EDITOR->wordHelp());
    helpViewer->showNormal();
    helpViewer->activateWindow();
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About \"%1\"").arg(PROGNAME),
                             tr("<h3>%1 v%2</h3>"
                                "<p>Based on Qt v%3</p>"
                                "<p>Built on %4 at %5</p>"
                                "<p>Author: <a href='mailto:novikovag@gmail.com?subject=%6'>"
                                    "Novikov Artem Gennadievich</a></p>"
                                "<p>Project pages: "
                                    "<a href='http://code.google.com/p/clipseditor/'>"
                                    "code.google</a>, "
                                    "<a href='https://sourceforge.net/projects/clipseditor/'>"
                                    "sourceforge</a>"
                                    "</p>"
                                "<p>License: <a href='http://www.gnu.org/licenses/gpl.html'>"
                                    "GNU GPL v3</a></p>"
                                "<hr>"
                                "<p>See also <a href='https://sourceforge.net/projects/clipsshell/'>CLIPSShell</a> "
                                    "a CLIPS development environment with UTF8 support</p>")
                                .arg(PROGNAME, VERSION, QT_VERSION_STR, __DATE__, __TIME__, PROGNAME));
}

void MainWindow::tabContextMenu(const QPoint& point)
{
    int index = tabWidget->tabBar()->tabAt(point);

    if (index != -1) {
        tabWidget->setCurrentIndex(index);

        /*
        QMenu menu(this);

        menu.addAction(tr("&Close"),this, SLOT(closeFile())); //, QKeySequence::Close);
        menu.exec(tabWidget->tabBar()->mapToGlobal(point));
        */

        closeFile();
    }
}

void MainWindow::modificationChanged(bool changed)
{
    QString str = tabWidget->tabText(tabWidget->currentIndex());
    QColor color;

    if (str[str.length() - 1] == '*') {

        if (!changed)
            str.resize(str.length() - 1);

    } else if (changed) {
        str  += '*';
        color = Qt::darkRed;
    }

    tabWidget->tabBar()->setTabTextColor(tabWidget->currentIndex(), color);
    tabWidget->setTabText(tabWidget->currentIndex(), str);

    saveAct->setEnabled(changed);
}

void MainWindow::currentChanged(int index)
{
    if (index == -1) {
        newNumber = 0;
        newFile();
        return;
    }

    saveAct->setEnabled(EDITOR->document()->isModified());
    connect(EDITOR, SIGNAL(modificationChanged(bool)), SLOT(modificationChanged(bool)), Qt::UniqueConnection);
}

void MainWindow::updateRecentFiles()
{
    int i = 0;

    QStringListIterator it(config->recentFiles);

    while (it.hasNext()) {
        QString fileName = it.next();

        if (QFileInfo(fileName).exists()) {
            recentFileActs[i]->setText(tr("&%1: %2").arg(i + 1).arg(fileName));
            recentFileActs[i]->setData(fileName);
            recentFileActs[i]->setVisible(true);
            i++;
        } else {
            config->recentFiles.removeAll(fileName);
        }
    }

    separatorAct->setVisible(i);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    int index = tabWidget->currentIndex(); // maybeSave()

    for (int i = 0; i < tabWidget->count(); i++) {
        tabWidget->setCurrentIndex(i);

        if (!maybeSave()) {
            event->ignore();
            return;
        }
    }

    config->lastFile = tabWidget->widget(index)->windowFilePath();

    config->openFiles.clear();

    for (int i = 0; i < tabWidget->count(); i++)
        if (!tabWidget->widget(i)->windowFilePath().isEmpty())
            config->openFiles.append(tabWidget->widget(i)->windowFilePath());

    event->accept();
}

bool MainWindow::maybeSave()
{
    if (EDITOR->document()->isModified()) {
        QMessageBox::StandardButton ret = QMessageBox::warning(this, tr(PROGNAME),
                                                               tr("The file has been modified.\n"
                                                                  "Do you want to save your changes?"),
                                                               QMessageBox::Save |
                                                               QMessageBox::Discard |
                                                               QMessageBox::Cancel);
        if (ret == QMessageBox::Save)
            return saveFile();
        else if (ret == QMessageBox::Cancel)
            return false;
    }

    return true;
}

void MainWindow::setCurrentFile(QString& fileName)
{
    fileName = QDir::toNativeSeparators(fileName);

    tabWidget->setTabText(tabWidget->currentIndex(), QFileInfo(fileName).fileName());
    tabWidget->currentWidget()->setWindowFilePath(fileName);

    config->recentFiles.removeAll(fileName);
    config->recentFiles.prepend(fileName);

    if (config->recentFiles.size() > config->maxRecentFiles)
        config->recentFiles.removeLast();

    lastPath = QFileInfo(fileName).path();
}

void MainWindow::loadFile(QString& fileName)
{
    QFile file(fileName);

    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr(PROGNAME),
                             tr("Cannot read file %1:\n%2")
                             .arg(fileName, file.errorString()));
    } else {
        QApplication::setOverrideCursor(Qt::WaitCursor);

        QTextStream in(&file);
        in.setCodec("UTF-8");

        EDITOR->setPlainText(in.readAll());
        modificationChanged(false);

        QApplication::restoreOverrideCursor();

        setCurrentFile(fileName);
    }
}

bool MainWindow::saveFile(QString &fileName)
{
    QFile file(fileName);

    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, tr(PROGNAME),
                             tr("Cannot write file %1:\n%2")
                             .arg(fileName, file.errorString()));
        return false;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");

    QApplication::setOverrideCursor(Qt::WaitCursor);

    out << EDITOR->toPlainText();

    QApplication::restoreOverrideCursor();

    setCurrentFile(fileName);

    EDITOR->document()->setModified(false);

    return true;
}

QString MainWindow::currentPath()
{
    if (!tabWidget->currentWidget()->windowFilePath().isEmpty() &&
        QDir(QFileInfo(tabWidget->currentWidget()->windowFilePath()).path()).exists())
        return QFileInfo(tabWidget->currentWidget()->windowFilePath()).path();

    if (!lastPath.isEmpty() && QDir(lastPath).exists())
        return lastPath;

    return QDir::homePath();
}

void MainWindow::addDock(QWidget* widget, QAction* action, QString title, Qt::DockWidgetArea area, Qt::DockWidgetAreas areas)
{
    QDockWidget* dock = new QDockWidget(title, this);

    action->setData(reinterpret_cast<int>(dock));

    dock->setObjectName(QString("dock%1").arg(docks.count()));
    dock->setAllowedAreas(areas);
    dock->setWidget(widget);

    addDockWidget(area, dock);

    connect(dock, SIGNAL(visibilityChanged(bool)), SLOT(showDock()));

    docks[dock->objectName()] = action;
}

void MainWindow::showDock()
{
    if (dynamic_cast<QAction*>(sender())) {
        reinterpret_cast<QDockWidget*>(static_cast<QAction*>(sender())->data().toInt())->setVisible(true);
    } else {
        QDockWidget* dock = static_cast<QDockWidget*>(sender());
        docks[dock->objectName()]->setEnabled(dock->isHidden());
    }
}


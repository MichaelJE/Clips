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

#include "helpviewer.h"

HelpViewer::HelpViewer(Config* config) : config(config)
{
    dir.setPath(QDir::tempPath() + "/clipshelp_" + QString::number(QCoreApplication::applicationPid()));
    dir.mkdir(dir.path());

    qhc.setFileName(dir.filePath("clipshelp.qhc"));
    qch.setFileName(dir.filePath("clipshelp.qch"));

    QFile::copy(":/help/clipshelp.qhc", qhc.fileName());
    QFile::copy(":/help/clipshelp.qch", qch.fileName());

    helpEngine = new QHelpEngine(qhc.fileName(), this);
    helpEngine->setupData();

    helpBrowser = new HelpBrowser(helpEngine, this);
    helpBrowser->setSource(QUrl("qthelp://clips-6.24/doc/index.html"));

    tabWidget = new QTabWidget(this);
    tabWidget->addTab(static_cast<QWidget*>(helpEngine->contentWidget()), tr("Content"));
    tabWidget->addTab(static_cast<QWidget*>(helpEngine->indexWidget()), tr("Index"));

    QSplitter* splitter = new QSplitter(this);

    splitter->addWidget(tabWidget);
    splitter->addWidget(helpBrowser);

    QList<int> sizes;
    sizes << 400 << 600;

    splitter->setSizes(sizes);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(splitter);

    setWindowFlags(Qt::WindowMinMaxButtonsHint);
    setWindowTitle(tr("CLIPS Help"));

    restoreGeometry(config->helpWindowGeometry);

    connect(static_cast<QWidget*>(helpEngine->contentWidget()),
            SIGNAL(linkActivated(const QUrl&)), helpBrowser, SLOT(setSource(const QUrl&)));
    connect(static_cast<QWidget*>(helpEngine->indexWidget()),
            SIGNAL(linkActivated(const QUrl&, const QString&)), helpBrowser, SLOT(setSource(const QUrl&)));
}

HelpViewer::~HelpViewer()
{
    helpEngine->setCollectionFile("");

    qhc.setPermissions(QFile::WriteUser);
    qch.setPermissions(QFile::WriteUser);

    qhc.remove();
    qch.remove();

    dir.rmdir(dir.path());

    config->helpWindowGeometry = saveGeometry();
}

void HelpViewer::setIndex(const QString& word)
{
    QHelpIndexModel* model = helpEngine->indexModel();

    for (int i = 0; i < model->rowCount() && !word.isEmpty(); i++)
        if (model->stringList().at(i) == word) {
            helpEngine->indexWidget()->scrollTo(model->index(i), QAbstractItemView::PositionAtTop);
            helpEngine->indexWidget()->selectionModel()->setCurrentIndex(model->index(i), QItemSelectionModel::SelectCurrent);
            helpEngine->indexWidget()->activateCurrentItem();
            tabWidget->setCurrentIndex(1);

            break;
        }
}

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

#include "snippets.h"

Snippets::Snippets(Config* config, QWidget* parent)
    : QWidget(parent), config(config)
{
    setupUi(this);

    QPalette palette;
    palette.setColor(QPalette::Base, palette.color(QPalette::Window));
    currentPathLine->setPalette(palette);
    snippetText->setPalette(palette);

    splitter = new QSplitter(Qt::Vertical);
    splitter->addWidget(snippetList);
    splitter->addWidget(snippetText);
    splitter->restoreState(config->snippetSplitter);
    verticalLayout_1->addWidget(splitter);

    if (QDir().exists(config->snippetPath))
        read(config->snippetPath);
    else
        read(QDir::currentPath());

    connect(changeDirButton, SIGNAL(clicked()), SLOT(changeDir()));
    connect(snippetList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), SLOT(doubleClicked(QListWidgetItem*)));
    connect(snippetList, SIGNAL(itemClicked(QListWidgetItem*)), SLOT(clicked(QListWidgetItem*)));
}

Snippets::~Snippets()
{
    config->snippetSplitter = splitter->saveState();
    config->snippetPath = currentPathLine->text();
}

void Snippets::changeDir()
{
    QString dirName = QFileDialog::getExistingDirectory(this, tr("Snippets Directory"), currentPathLine->text(), QFileDialog::ShowDirsOnly);

    if (!dirName.isEmpty())
        read(dirName);
}

void Snippets::read(QString dirName)
{
    currentPathLine->setText(dirName);

    snippetList->clear();
    snippetText->clear();

    QFileInfoList list = QDir(dirName).entryInfoList(QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot);

    for (int i = 0; i < list.size(); ++i) {
        QFileInfo fileInfo = list.at(i);

        QListWidgetItem* item = new QListWidgetItem(fileInfo.fileName(), snippetList);
        item->setData(Qt::UserRole, fileInfo.absoluteFilePath());
    }
}

void Snippets::clicked(QListWidgetItem* item)
{
    QFile file(item->data(Qt::UserRole).toString());

    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr(PROGNAME),
                             tr("Cannot read file %1:\n%2")
                             .arg(item->data(Qt::UserRole).toString(),
                                  file.errorString()));
        snippet.clear();
    } else {
        QTextStream in(&file);
        in.setCodec("UTF-8");

        snippet = in.readAll();
    }

    snippetText->setPlainText(snippet);
}

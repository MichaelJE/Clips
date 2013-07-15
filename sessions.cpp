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

#include "sessions.h"

Sessions::Sessions(Config* config, QTabWidget* tabWidget, QWidget* parent)
    : QWidget(parent), config(config), tabWidget(tabWidget)
{
    setupUi(this);

    QPalette palette;
    palette.setColor(QPalette::Base, palette.color(QPalette::Window));
    sessionText->setPalette(palette);

    splitter = new QSplitter(Qt::Vertical);
    splitter->addWidget(sessionList);
    splitter->addWidget(sessionText);
    splitter->restoreState(config->sessionSplitter);

    verticalLayout_1->addWidget(splitter);

    QMapIterator<QString, QVariant> it(config->sessions);

    while (it.hasNext()) {
        it.next();

        QListWidgetItem* item = new QListWidgetItem(it.key(), sessionList);
        item->setData(Qt::UserRole, it.value());
    }

    connect(addButton,    SIGNAL(clicked()), SLOT(add()));
    connect(removeButton, SIGNAL(clicked()), SLOT(remove()));
    connect(sessionList,  SIGNAL(itemSelectionChanged()), SLOT(select()));
    connect(sessionList,  SIGNAL(itemDoubleClicked(QListWidgetItem*)), SLOT(clicked(QListWidgetItem*)));
}

Sessions::~Sessions()
{
    config->sessions.clear();

    for (int i = 0; i < sessionList->count(); i++)
        config->sessions[sessionList->item(i)->text()] = sessionList->item(i)->data(Qt::UserRole);

    config->sessionSplitter = splitter->saveState();
}

void Sessions::add()
{
    if (!sessionEdit->text().isEmpty() && sessionList->findItems(sessionEdit->text(), Qt::MatchExactly).empty()) {
        QString txt;

        for (int i = 0; i < tabWidget->count(); i++)
            if (!tabWidget->widget(i)->windowFilePath().isEmpty()) {
                if (!txt.isEmpty())
                    txt += "\n";

                txt += tabWidget->widget(i)->windowFilePath();
            }

        if (!txt.isEmpty()) {
            QListWidgetItem* item = new QListWidgetItem(sessionEdit->text(), sessionList);
            item->setData(Qt::UserRole, txt);
            sessionEdit->clear();
        }
    }
}

void Sessions::remove()
{
    QListIterator<QListWidgetItem*> it(sessionList->selectedItems());

    while (it.hasNext())
        delete it.next();
}

void Sessions::select()
{
    sessionText->clear();

    if (sessionList->currentItem())
        sessionText->insertPlainText(sessionList->currentItem()->data(Qt::UserRole).toString());
}

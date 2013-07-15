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

#ifndef SNIPPETS_H
#define SNIPPETS_H

#include <QtGui>

#include "ui_snippets.h"

#include "config.h"

class Snippets : public QWidget, private Ui::Snippets
{
    Q_OBJECT

public:
    Snippets(Config*, QWidget* = 0);
   ~Snippets();

signals:
    void insert(QString);

private slots:
    void changeDir();
    void clicked(QListWidgetItem*);
    void doubleClicked(QListWidgetItem*) { emit insert(snippet); }

private:
    void read(QString);

    QString    snippet;
    QSplitter* splitter;
    Config*    config;
};


#endif // SNIPPETS_H

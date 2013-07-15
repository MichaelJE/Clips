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

#ifndef SESSIONS_H
#define SESSIONS_H

#include <QtGui>

#include "ui_sessions.h"

#include "config.h"

class Sessions : public QWidget, private Ui::Sessions
{
    Q_OBJECT

public:
    Sessions(Config*, QTabWidget*, QWidget* = 0);
   ~Sessions();

signals:
    void load(QStringList, bool);

private slots:
    void add();
    void remove();
    void select();
    void clicked(QListWidgetItem* item) { emit load(item->data(Qt::UserRole).toString().split("\n"), leaveOpenCheck->isChecked()); }

private:
    QSplitter*  splitter;
    QTabWidget* tabWidget;
    Config* 	config;
};

#endif // SESSIONS_H

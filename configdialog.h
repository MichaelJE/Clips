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

#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QtGui>

#include "ui_configdialog.h"

#include "config.h"
#include "codeeditor.h"

class ConfigDialog : public QDialog, private Ui::ConfigDialog
{
    Q_OBJECT

public:
    ConfigDialog(Config*, QWidget* = 0);

private slots:
    void changeFont();
    void changeScheme(QListWidgetItem*);
    void changeColor();

    void accept();
    void applay();

private:
     QString colorButtonStyleSheet(const QColor& color)
     {
         return QString("border: 1px solid black; background: %1").arg(color.name());
     }

     Config* config;
};


#endif // CONFIGDIALOG_H

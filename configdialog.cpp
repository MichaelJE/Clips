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

#include "configdialog.h"

ConfigDialog::ConfigDialog(Config* config, QWidget* parent)
    : QDialog(parent), config(config)
{
    setupUi(this);

    languageCombo->addItems(config->languages.keys());
    languageCombo->setCurrentIndex(languageCombo->findText(config->language));

    fontFamilyCombo->setCurrentFont(QFont(config->fontFamily));
    fontSizeSpin->setValue(config->fontSize);
    tabIndentsCheck->setChecked(config->tabIndents);
    autoIndentCheck->setChecked(config->autoIndent);
    backUnindentCheck->setChecked(config->backUnindent);
    spaceTabsCheck->setChecked(config->spaceTabs);
    indentSizeSpin->setValue(config->indentSize);
    tabSizeSpin->setValue(config->tabSize);
    whitespacesCheck->setChecked(config->whitespaces);

    QMapIterator<QString, QTextCharFormat> it(config->colorScheme);

    while (it.hasNext()) {
        it.next();

        QListWidgetItem* item = new QListWidgetItem(it.key(), schemeList);

        item->setForeground(it.value().foreground());
        item->setBackground(it.value().background());
        item->setFont(it.value().font());
    }

    schemeList->setCurrentRow(0);
    foregroundTool->setStyleSheet(colorButtonStyleSheet(schemeList->currentItem()->foreground().color()));
    backgroundTool->setStyleSheet(colorButtonStyleSheet(schemeList->currentItem()->background().color()));
    boldCheck->setChecked(schemeList->currentItem()->font().bold());
    italicCheck->setChecked(schemeList->currentItem()->font().italic());

    connect(foregroundTool, SIGNAL(clicked()), SLOT(changeColor()));
    connect(backgroundTool, SIGNAL(clicked()), SLOT(changeColor()));
    connect(boldCheck,      SIGNAL(clicked()), SLOT(changeFont()));
    connect(italicCheck,    SIGNAL(clicked()), SLOT(changeFont()));
    connect(schemeList,     SIGNAL(itemClicked(QListWidgetItem*)), SLOT(changeScheme(QListWidgetItem*)));
    connect(buttonBox->button(QDialogButtonBox::Apply), SIGNAL(clicked()), SLOT(applay()));

    adjustSize();
    setFixedSize(size());
}

void ConfigDialog::accept()
{
    applay();
    QDialog::accept();
}

void ConfigDialog::applay()
{
    if (config->language != languageCombo->currentText())
        QMessageBox::information(this, tr("Restart required"),
                                       tr("The language change will take effect after a restart editor"));

    config->language     = languageCombo->currentText();
    config->fontFamily   = fontFamilyCombo->currentFont().family();
    config->fontSize     = fontSizeSpin->value();
    config->tabIndents   = tabIndentsCheck->isChecked();
    config->autoIndent   = autoIndentCheck->isChecked();
    config->backUnindent = backUnindentCheck->isChecked();
    config->spaceTabs    = spaceTabsCheck->isChecked();
    config->tabSize      = tabSizeSpin->value();
    config->indentSize   = indentSizeSpin->value();
    config->whitespaces  = whitespacesCheck->isChecked();

    for (int i = 0; i < schemeList->count(); i++) {
        QListWidgetItem* item = schemeList->item(i);
        QTextCharFormat  fmt;

        fmt.setForeground(item->foreground());
        fmt.setBackground(item->background());
        fmt.setFontItalic(item->font().italic());
        fmt.setFontWeight(item->font().weight());
        config->colorScheme[item->text()] = fmt;
    }

    config->reconfig(Config::Editor);
}

void ConfigDialog::changeFont()
{
    QListWidgetItem* item = schemeList->currentItem();

    QFont font = item->font();

    font.setItalic(italicCheck->isChecked());
    font.setBold(boldCheck->isChecked());

    item->setFont(font);
}

void ConfigDialog::changeScheme(QListWidgetItem* item)
{
    boldCheck->setChecked(item->font().bold());
    italicCheck->setChecked(item->font().italic());

    foregroundTool->setStyleSheet(colorButtonStyleSheet(const_cast<QColor&>(item->foreground().color())));
    backgroundTool->setStyleSheet(colorButtonStyleSheet(const_cast<QColor&>(item->background().color())));
}

void ConfigDialog::changeColor()
{
    QColor color = QColorDialog::getColor(static_cast<QToolButton*>(sender())->palette().background().color());

    if (!color.isValid())
        return;

    if (sender() == foregroundTool)
        schemeList->currentItem()->setForeground(color);
    else
        schemeList->currentItem()->setBackground(color);

    static_cast<QToolButton*>(sender())->setStyleSheet(colorButtonStyleSheet(color));
}


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

#include "searchbar.h"

SearchBar::SearchBar(Config* config, QWidget* parent)
    : QWidget(parent), config(config)
{
    setupUi(this);

    findCombo->setMaxCount(config->maxHistory);
    findCombo->addItems(config->findHistory);
    findCombo->setCurrentIndex(-1);

    replaceCombo->setMaxCount(config->maxHistory);
    replaceCombo->addItems(config->replaceHistory);
    replaceCombo->setCurrentIndex(-1);

    matchCaseCheck->setChecked(config->matchCase);
    regExpCheck->setChecked(config->regExp);
    allFilesCheck->setChecked(config->allFiles);

    connect(findNextButton,        SIGNAL(clicked()), SLOT(clicked()));
    connect(findPreviousButton,    SIGNAL(clicked()), SLOT(clicked()));
    connect(replaceNextButton,     SIGNAL(clicked()), SLOT(clicked()));
    connect(replacePreviousButton, SIGNAL(clicked()), SLOT(clicked()));
    connect(replaceAllButton,      SIGNAL(clicked()), SLOT(clicked()));
}

SearchBar::~SearchBar()
{
    config->matchCase = matchCaseCheck->isChecked();
    config->regExp    = regExpCheck->isChecked();
    config->allFiles  = allFilesCheck->isChecked();

    config->findHistory.clear();

    for (int i = 0; i < findCombo->count(); i++)
        config->findHistory << findCombo->itemText(i);

    config->replaceHistory.clear();

    for (int i = 0; i < replaceCombo->count(); i++)
        config->replaceHistory << replaceCombo->itemText(i);
}

void SearchBar::clicked()
{
    if (findCombo->currentText().isEmpty())
        return;

    update(findCombo);

    int flags = 0;

    if (matchCaseCheck->isChecked())
        flags |= MatchCase;

    if (regExpCheck->isChecked())
        flags |= RegExp;

    if (allFilesCheck->isChecked())
        flags |= AllFiles;

    if (sender() == replaceAllButton)
        flags |= ReplaceAll;

    if (sender() == findPreviousButton || sender() == replacePreviousButton)
        flags |= Backward;

    if (sender() == findNextButton || sender() == findPreviousButton) {
        emit searchReplace(findCombo->currentText(), 0, flags);
    } else if (findCombo->currentText() != replaceCombo->currentText()) {
        update(replaceCombo);
        emit searchReplace(findCombo->currentText(), replaceCombo->currentText(), flags);
    }
}

void SearchBar::update(QComboBox* combo)
{
    int index = combo->findText(combo->currentText());

    if (index == -1)
        combo->insertItem(0, combo->currentText());
    else
        combo->setCurrentIndex(index);
}

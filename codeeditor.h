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

#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QtGui>

#include "config.h"
#include "highlighter.h"

#define FOLDBOXRECT(cy) QRect(lineNumWidth + foldBoxIndent, cy - foldBoxWidth / 2, foldBoxWidth, foldBoxWidth)
#define FULLRESIZE      resizeEvent(new QResizeEvent(QSize(0, 0), size()))
#define FONTWIDTH       fontMetrics().width(QLatin1Char('X'))

class Highlighter;

class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    // при добавлении флага, смотреть метод setBlockState(), поскольку
    // previousBlockState и previousState перезаписывают статус, также как
    // и contentsChange()
    enum BlockState {
        End        = 1,   // 00000001, конец блока
        Begin      = 2,   // 00000010, начало блока
        String     = 4,   // 00000100, признак незавершенной сторки (нет пары для ")
        Comment    = 8,   // 00001000, строка является внешним комментарием
        Nested     = 16,  // 00010000, признак вложенности
        Folded     = 32,  // 00100000, блок свернут
        Error      = 64,  // 01000000, строка содержит ошибку
        Rehighligh = 128, // 10000000, блок необходими переподсветить
        Empty      = -1   // 1111...., новый/пустой блок
    };

    const static int StateShift = 8; // позиция последнего значащего бита, после которого идет значение вложенности

    enum SearchFlag {  // QTextDocument::FindFlag
        Backward   = 1,
        MatchCase  = 2,
        WholeWords = 4,
        RegExp     = 8
    };

public:
    CodeEditor(Config*, QWidget* = 0);

    QString wordHelp();

    bool search(QString, int);
    bool replace(QString, QString, int);
    void replaceAll(QString, QString, int);

private slots:
    void foldAll()        { foldUnfoldAll(); }
    void unfoldAll()      { foldUnfoldAll(false); }
    void deleteSelected() { textCursor().removeSelectedText(); }
    void toggleComment();
    void toUpperCase()    { convertCase(); }
    void toLowerCase()	  { convertCase(false); }

    void blockCountChanged(int count) { lineNumDigits = qMax(2, QString::number(count).length()); } // 2 - разряда
    void contentsChange(int, int, int);
    void performCompletion();
    void insertCompletion(const QString&);
    void ensureCursorVisible();

    void reconfig(int = (Config::Init | Config::Editor));

private:
    void resizeEvent(QResizeEvent*);
    void paintEvent(QPaintEvent*);

    void keyPressEvent(QKeyEvent*);
    void mouseMoveEvent(QMouseEvent*);
    void mousePressEvent(QMouseEvent*);
    void contextMenuEvent(QContextMenuEvent*);

    bool eventFilter(QObject*, QEvent*);
    void extraAreaPaintEvent();
    void extraAreaMouseEvent(QMouseEvent*);

    int  setBlockState(QTextBlock&);
    QTextBlock findBlockByY(int);

    void foldUnfold(QTextBlock&);
    void foldUnfoldAll(bool = true);

    void convertCase(bool toUpper = true)
    {
        textCursor().insertText(toUpper ? textCursor().selectedText().toUpper() :
                                          textCursor().selectedText().toLower());
    }

    Highlighter*   highlighter;
    QCompleter*    completer;

    QTextBlock     pointedBlock;

    QWidget*       extraArea;

    int            lineNumDigits;
    int            lineNumWidth;

    int            foldBoxWidth;
    int            foldBoxIndent;
    int            foldBoxLength;

    Config*        config;
};

#endif // CODEEDITOR_H

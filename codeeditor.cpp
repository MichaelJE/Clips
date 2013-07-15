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

#include "codeeditor.h"

CodeEditor::CodeEditor(Config* config, QWidget* parent)
    : QPlainTextEdit(parent), config(config)
{
    highlighter = new Highlighter(config, this); // тормоз

    extraArea = new QWidget(this);
    extraArea->setCursor(Qt::PointingHandCursor);
    extraArea->setAutoFillBackground(true);
    extraArea->installEventFilter(this);

    completer = new QCompleter(config->keywordsSorted, this);
    completer->setWidget(this);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->setWrapAround(false);

    setLineWrapMode(QPlainTextEdit::NoWrap);
    setCursorWidth(2);
    blockCountChanged(0);
    setMouseTracking(true);

    reconfig();

    // не допускаем проваливание на последнем свернутом блоке
    connect(this,       SIGNAL(cursorPositionChanged()),       SLOT(ensureCursorVisible()));
    connect(this,       SIGNAL(blockCountChanged(int)),        SLOT(blockCountChanged(int)));
    connect(document(), SIGNAL(contentsChange(int, int, int)), SLOT(contentsChange(int, int, int)));
    connect(completer,  SIGNAL(activated(const QString&)),     SLOT(insertCompletion(const QString&)));
    connect(config,     SIGNAL(reread(int)),                   SLOT(reconfig(int)));
    connect(this,       SIGNAL(updateRequest(QRect, int)), extraArea, SLOT(update()));
}

void CodeEditor::resizeEvent(QResizeEvent* event)
{
    QPlainTextEdit::resizeEvent(event);

    lineNumWidth = FONTWIDTH * lineNumDigits;

    int width = lineNumWidth + foldBoxWidth + 2 * foldBoxIndent;

    setViewportMargins(width, 0, 0, 0);

    extraArea->setGeometry(contentsRect().x(), contentsRect().y(), width, contentsRect().height());
}

void CodeEditor::paintEvent(QPaintEvent* event)
{
    QPlainTextEdit::paintEvent(event);

    QPainter painter(viewport());
    painter.setPen(Qt::darkGray);

    QTextBlock block = firstVisibleBlock();

    QRectF rect;

    do {

        if (!block.isVisible())
            continue;

        rect = blockBoundingGeometry(block).translated(contentOffset());

        QTextLine line = block.layout()->lineAt(0);

        if (config->whitespaces) {
            QString txt = block.text();

            for (int i = 0; i < txt.length(); i++) {
                // rect.x() <- учитывая горизонтальный скролинг
                QPoint point(rect.x() + line.cursorToX(i), rect.y() + line.ascent());

                if (txt[i] == ' ')
                    painter.drawText(point, QChar(0x00b7));
                else if (txt[i] == '\t')
                    painter.drawText(point, QChar(0x21b9));
            }
        }

        int state = block.userState();

        if (!(state & Error) && state & Folded) {
            QRect collapseRect(rect.x() + line.rect().x() + line.naturalTextWidth() + FONTWIDTH * 2,
                               rect.y() + 2, FONTWIDTH * 6, line.height() - 4);

            painter.drawText(collapseRect, Qt::AlignCenter, state & Comment ? "...;" : "...)");
            painter.drawRoundedRect(collapseRect, 4, 6);
        }

    } while ((block = block.next()).isValid() && rect.y() < viewport()->height());
}

void CodeEditor::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return && event->modifiers() == Qt::SHIFT)
        return;

    if (event->key() == Qt::Key_E && event->modifiers() == Qt::CTRL) {
        QTextBlock block = textCursor().block();
        foldUnfold(block);
        FULLRESIZE;
        return;
    }

    if (event->key() == Qt::Key_Space && event->modifiers() == Qt::CTRL) {
        performCompletion();
        return;
    }

    if (event->key() == Qt::Key_Backspace && config->backUnindent) {
        QTextCursor cursor = textCursor();

        if (!cursor.hasSelection() && !cursor.atBlockStart() && cursor.block().text().          \
                                                                left(cursor.positionInBlock()). \
                                                                trimmed().isEmpty()) {
            cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor,
                                qMin(config->indentSize, cursor.positionInBlock()));
            cursor.removeSelectedText();
            return;
        }
    }

    if (event->key() == Qt::Key_Tab && config->tabIndents) {
        QTextCursor cursor = textCursor();

        if (cursor.hasSelection()) {

            QTextBlock block = document()->findBlock(cursor.selectionStart());

            int end = document()->findBlock(cursor.selectionEnd()).blockNumber();

            if (end - block.blockNumber()) {

                cursor.beginEditBlock();

                do {
                    cursor.setPosition(block.position(), QTextCursor::MoveAnchor);
                    cursor.insertText(QString().fill(' ', config->indentSize));
                } while ((block = block.next()).isValid() && block.blockNumber() <= end);

                cursor.endEditBlock();

                return;
            }

        } else if (textCursor().block().text().          \
                   left(textCursor().positionInBlock()). \
                   trimmed().isEmpty()) {
                   textCursor().insertText(QString().fill(' ', config->indentSize));
                   return;
        }
    }

    if (event->key() == Qt::Key_Tab && config->spaceTabs) {
        textCursor().insertText(QString().fill(' ', config->tabSize));
        return;
    }

    if (completer->popup()->isVisible()) {
        switch (event->key()) {
            case Qt::Key_Up:
            case Qt::Key_Down:
            case Qt::Key_Enter:
            case Qt::Key_Return:
            case Qt::Key_Escape:
                event->ignore();
                return;
        }
    }

    QPlainTextEdit::keyPressEvent(event);

    if (completer->popup()->isVisible())
        performCompletion();

    if (event->key() == Qt::Key_Return && config->autoIndent) {
        int state = textCursor().block().userState();

        if (!(state & Error) && (state & Nested)) {
            QString txt = textCursor().block().previous().text();

            int i = 0;

            while (txt[i].isSpace()) ++i;

            int previousBlockState = textCursor().block().previous().userState();

            if (!(previousBlockState & Error) && previousBlockState & Begin)
                i += config->indentSize;

            textCursor().insertText(QString().fill(' ', i));
        }
    }
}

void CodeEditor::mouseMoveEvent(QMouseEvent* event)
{
    QTextBlock block = findBlockByY(event->pos().y());

    QRect collapseRect;

    if (block.isValid()) {
        QRectF    rect = blockBoundingGeometry(block).translated(contentOffset());
        QTextLine line = block.layout()->lineAt(0);

        collapseRect = QRect(rect.x() + line.rect().x() + line.naturalTextWidth() + FONTWIDTH * 2,
                             rect.y() + 2, FONTWIDTH * 6, line.height() - 4);
    }

    int state = block.userState();

    if (!(state & Error) && state & Folded && collapseRect.contains(event->pos())) {
        pointedBlock = block;

        viewport()->setCursor(Qt::PointingHandCursor);

        QString str;

        while ((block = block.next()).isValid() && !block.isVisible()) {

            if (str.count() > 1)
                str += "\n";

            if (block.blockNumber() - pointedBlock.blockNumber() > 50) {
                str += "..."; // "\n...";
                break;
            }

            str += block.text();
        }

        QToolTip::showText(event->globalPos(), str, this);
    } else {
        pointedBlock = QTextBlock();
        viewport()->setCursor(Qt::IBeamCursor);
    }

    QPlainTextEdit::mouseMoveEvent(event);
}

void CodeEditor::mousePressEvent(QMouseEvent* event)
{
    if (pointedBlock.isValid()) {
        foldUnfold(pointedBlock);
        pointedBlock = QTextBlock();
        viewport()->setCursor(Qt::IBeamCursor);

        FULLRESIZE;
    } else {
        QPlainTextEdit::mousePressEvent(event);
    }
}

void CodeEditor::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu(this);

    QAction* act = menu.addAction(tr("&Undo"), this, SLOT(undo()));       //, QKeySequence::Undo);
    act->setEnabled(document()->isUndoAvailable());
    act = menu.addAction(tr("&Redo"), this, SLOT(redo()));                //, QKeySequence::Redo);
    act->setEnabled(document()->isRedoAvailable());
    menu.addSeparator();
    act = menu.addAction(tr("Cu&t"),       this, SLOT(cut()));            //, QKeySequence::Cut);
    act->setEnabled(textCursor().hasSelection());
    act = menu.addAction(tr("&Copy"),      this, SLOT(copy()));           //, QKeySequence::Copy);
    act->setEnabled(textCursor().hasSelection());
    act = menu.addAction(tr("&Paste"),     this, SLOT(paste()));          //, QKeySequence::Paste);
    act->setEnabled(canPaste());
    act = menu.addAction(tr("Delete"),     this, SLOT(deleteSelected())); //, QKeySequence::Delete);
    act->setEnabled(textCursor().hasSelection());
    act = menu.addAction(tr("Select All"), this, SLOT(selectAll()));      //, QKeySequence::SelectAll);
    act->setEnabled(!document()->isEmpty());
    menu.addSeparator();
    menu.addAction(tr("Fold All"),   this, SLOT(foldAll()));
    menu.addAction(tr("Unfold All"), this, SLOT(unfoldAll()));
    menu.addSeparator();
    menu.addAction(tr("Toggle comment"), this, SLOT(toggleComment()));
    menu.addSeparator();
    menu.addAction(tr("UPPERCASE"), this, SLOT(toUpperCase()));
    menu.addAction(tr("lowercase"), this, SLOT(toLowerCase()));

    menu.exec(event->globalPos());
}

bool CodeEditor::eventFilter(QObject* obj, QEvent* event)
{
    switch (event->type()) {
        case QEvent::Paint:
            extraAreaPaintEvent();
            return true;
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonDblClick:
            extraAreaMouseEvent(static_cast<QMouseEvent*>(event));
            return true;
        default:
            return QPlainTextEdit::eventFilter(obj, event);
    }
}

void CodeEditor::extraAreaPaintEvent()
{
    QTextBlock block = firstVisibleBlock();

    QPainter painter(extraArea);
    QPen 	 pen  = painter.pen();
    QFont 	 font = painter.font();
    bool 	 bold = font.bold();

    int y  = 0;
    int cx = lineNumWidth + foldBoxIndent + foldBoxWidth / 2;  // центр маркера блока по x

    do {
        if (!block.isVisible())
            continue;

        QRectF rect = blockBoundingGeometry(block);

        y = rect.translated(contentOffset()).y();

        if (block == textCursor().block()) {
            painter.setPen(Qt::yellow);
            font.setBold(!bold);
        } else {
            font.setBold(bold);
        }

        painter.setFont(font);
        painter.drawText(0, y, lineNumWidth, fontMetrics().height(), Qt::AlignRight,
                         QString::number(block.blockNumber() + 1)); // номер строки

        painter.setPen(pen);

        int state = block.userState();

        int cy    = y + fontMetrics().height() / 2; // центр маркера по Y

        if (!(state & Error) && !(state & Begin && state & End && !(state & Nested))) {

            if (state & Begin) {

                if (state & Comment)
                    painter.drawEllipse(FOLDBOXRECT(cy));
                else
                    painter.drawRoundedRect(FOLDBOXRECT(cy), 7, 3);

                if (!(!(state & Nested) && state & Folded))
                    painter.drawLine(cx, cy + foldBoxWidth / 2, cx, y + rect.height());  // края маркера вниз

                if (state & Nested)
                    painter.drawLine(cx, y, cx, cy - foldBoxWidth / 2);                  // края маркера вверх

                painter.drawLine(cx - foldBoxLength, cy, cx + foldBoxLength, cy);        // горизонтальная линия внутри маркера

                if (state & Folded)
                    painter.drawLine(cx, cy - foldBoxLength, cx, cy + foldBoxLength);    // вертикальная линия внутри маркера

            } else if (state & End) {
                painter.drawLine(cx, cy, cx + foldBoxLength, cy);                        // от центра вправо

                if (state & Nested)
                    painter.drawLine(cx, y, cx, y + rect.height());                      // вертикальная линия
                else
                    painter.drawLine(cx, y, cx, cy);                                     // от центра вверх

            } else if (state & Nested) {
                painter.drawLine(cx, y, cx, y + rect.height());                          // вертикальная линия
            }
        }

        // в paintEvent не работает
        if (state != Empty && state & Rehighligh) {
            highlighter->rehighlightBlock(block);
            block.setUserState(state & ~Rehighligh);
        }

    } while ((block = block.next()).isValid() && y < viewport()->height());
}

void CodeEditor::extraAreaMouseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        QTextBlock block = findBlockByY(event->pos().y());

        if (FOLDBOXRECT(blockBoundingGeometry(block).translated(contentOffset()).y() +
                        fontMetrics().height() / 2).contains(event->pos())) {
            foldUnfold(block);
            ensureCursorVisible();
            FULLRESIZE;
        }
    }
}

int CodeEditor::setBlockState(QTextBlock& block)
{
    int previousBlockState = block.previous().userState();

    if (previousBlockState & Error)
        previousBlockState = 0;

    bool inString = previousBlockState & String;

    int previousBraceDepth = previousBlockState >> StateShift;

    int braceDepth = previousBraceDepth;

    QString txt = block.text();

    int state = 0;
    int i     = 0;

    while (txt[i].isSpace()) ++i;

    if (txt[i] == ';') {

        if (!previousBraceDepth || previousBlockState & Comment) {
            state |= Comment; // только за предалами блока кода

            if (previousBlockState & Comment) {

                if (braceDepth) {
                    braceDepth--;
                } else {
                    previousBraceDepth = 1;

                    previousBlockState = (previousBlockState & End ? Nested : Begin) |
                                         (previousBlockState & Folded) |
                                         (previousBlockState & Rehighligh);

                    previousBlockState |= (previousBraceDepth << StateShift) | Comment;

                    block.previous().setUserState(previousBlockState);
                }

                state |= End;
            }
        }

    } else {
        // коррекция блока после удаления строки комментария
        if (previousBlockState & Comment && previousBlockState & (Nested | Begin)) {
            previousBraceDepth = 0;
            braceDepth = 0;

            previousBlockState  = previousBlockState & Nested ? End : 0;
            previousBlockState |= (previousBraceDepth << StateShift) | Comment;

            block.previous().setUserState(previousBlockState);
        }

        if (txt[i].isNull() && !previousBraceDepth) {      // пустая строка
            //qDebug() << "*** Error #1 'empty string'";
            state = Empty;
        } else if (txt[i] != '(' && !previousBraceDepth) { // начальный символ вне блока не '(' или ';'
            //qDebug() << "*** Error #2 'bad begin'";
            braceDepth = i; // позиция начала ошибки
            state |= Error;
        } else {
            while (i < txt.length()) {

                if (txt[i] == '"' && !(i && txt[i - 1] == '\\')) {
                    inString = !inString;
                } else if (!inString) {

                    if (txt[i] == ';') // комментарий в блоке кода
                        break;

                    if (txt[i] == '(')
                        braceDepth++;
                    else if (txt[i] == ')')
                        braceDepth--;

                    if (braceDepth == -1) {
                        //qDebug() << "*** Error #3 ') mismatch'";
                        braceDepth = i;
                        state |= Error;
                        break;
                    }
                }

                i++;
            } // while
        }
    }

    if (!(state & (Error | Comment))) {

        if (inString)
            state |= String;

        if (braceDepth > previousBraceDepth || !previousBraceDepth)
            state |= Begin;

        if (braceDepth < previousBraceDepth || !braceDepth)
            state |= End;

        if (previousBraceDepth && braceDepth)
            state |= Nested;
    }

    state |= (braceDepth << StateShift);

    int previousState = block.userState();

    if (previousState != Empty)
        state |= (previousState & Folded) | (previousState & Rehighligh);

//    qDebug("Block[%d]\n"
//           "\tDepth %d/%d State %d/%d New %d",
//           block.blockNumber(),
//           braceDepth, previousBraceDepth,
//           state,      block.userState(),
//           (bool)(previousState != state));

//    if (state & Error)
//        qDebug("\tError position %d", braceDepth);
//    else
//        qDebug("\tEnd:%d Begin:%d String:%d Comment:%d Nested:%d Folded:%d",
//               (bool)(state & End),    (bool)(state & Begin),
//               (bool)(state & String), (bool)(state & Comment),
//               (bool)(state & Nested), (bool)(state & Folded));

//    if (previousState & Error)
//        qDebug("\t[P] Error position %d", braceDepth);
//    else
//        qDebug("\t[P] End:%d Begin:%d String:%d Comment:%d Nested:%d Folded:%d",
//               (bool)(previousState & End),    (bool)(previousState & Begin),
//               (bool)(previousState & String), (bool)(previousState & Comment),
//               (bool)(previousState & Nested), (bool)(previousState & Folded));

    block.setUserState(state);

    return state;
}

void CodeEditor::contentsChange(int pos, int, int)
{
    QTextBlock block = textCursor().block();

    // сдвиг блока клавишей Return
    // состояние предыдущего блока может быть не верным
    if (block.userState() == Empty)
        block.setUserState(block.previous().userState() | Rehighligh);

    block = document()->findBlock(pos);

    int startBlockNum = block.blockNumber();

    bool forceUnfold = false;

    while (block.isValid()) {
        int previousState = block.userState();
        int state = setBlockState(block);

        if (!(previousState & Error) && !(state & Error) &&
            previousState & Comment  && state & Comment) {

            QTextBlock next = block.next();

            int nextBlockState = next.userState();
            setBlockState(next);
            next.setUserState(nextBlockState); // в начальное состояние

            // правильное состояние комментария известно
            // только после обработки последующей строки
            state = block.userState();
        }

        if (state != previousState) {

            if (!forceUnfold) { // разворачиваем предыдущие блоки
                QTextBlock previous = block.previous();

                while (previous.isValid()) {
                    int state = previous.userState();

                    if (!(state & Error) && state & Folded)
                        previous.setUserState(state & ~Folded);

                    if (state & Error || !(state & Nested) || previous.isVisible())
                        break;

                    previous.setVisible(true);
                    previous = previous.previous();
                }
            }

            forceUnfold = true;
        } else if (block.blockNumber() > startBlockNum && // не начальный блок
                   state & Begin && !(state & Nested)) {  // (state & Begin ...) пропускаем End главного блока
          break;                                          // без state & Error, иначе срабатывает между блоками
        }                                                 // при первом обновлении

        if (forceUnfold) { // разворачиваем последующие блоки

            if (!(state & Error) && state & Folded)
                state &= ~Folded;

            block.setVisible(true);
        }

        block.setUserState(state | Rehighligh);

        block = block.next();
    }
}

void CodeEditor::foldUnfold(QTextBlock& block)
{
    int state = block.userState();

    if (state & Error || state & End || !(state & Begin) || // (state & End) для однострочного блока ()
        document()->lastBlock() == block)
        return;

    bool unfolding = state & Folded;

    if (unfolding)
        block.setUserState(block.userState() & ~Folded);
    else
        block.setUserState(block.userState() | Folded);

    int previousBlockState = block.previous().userState();
    int endBraceDepth = previousBlockState & Error ? 0 : previousBlockState >> StateShift;

    int braceDepth;
    int skipDepth = 0;

    while ((block = block.next()).isValid()) {
        int state = block.userState();
        braceDepth = state >> StateShift;

        if (unfolding) {

            if (state & Begin && !skipDepth && state & Folded) {
                skipDepth = block.previous().userState() >> StateShift;
            } else if (skipDepth) {

                if (braceDepth == skipDepth)
                    skipDepth = 0;

                continue;
            }
        }

        block.setVisible(unfolding);

        if (braceDepth <= endBraceDepth)
            break;
    }
}

void CodeEditor::foldUnfoldAll(bool folding)
{
    QTextBlock block = document()->firstBlock();

    do {
        int state = block.userState();

        if (!block.isVisible() || state & Error || state & Nested ||
            (state & Folded && folding) || (!(state & Folded) && !folding))
            continue;

        foldUnfold(block);

    } while ((block = block.next()).isValid());

    ensureCursorVisible();
    FULLRESIZE;
}

void CodeEditor::performCompletion()
{
    QString txt = textCursor().block().text();

    int i = textCursor().positionInBlock();

    // слова включают символ '-' (завершающиe '*', '$' не учитываем)
    // обходимся без QTextCursor::WordUnderCursor
    while (--i >= 0 && (QChar(txt[i]).isLetter() || txt[i] == '-'));

    i++;

    QString prefix = txt.mid(i, textCursor().positionInBlock() - i);

    completer->setCompletionPrefix(prefix);

    if ((!prefix.length() && completer->popup()->isVisible()) ||
        completer->completionCount() <= 1) {

        completer->popup()->hide();

        if (completer->completionCount() == 1)
            insertCompletion(completer->currentCompletion());

    } else {
        completer->popup()->setCurrentIndex(completer->completionModel()->index(0, 0));

        QRect rect = cursorRect();

        rect.setX(rect.x() + extraArea->width());
        rect.setWidth(completer->popup()->sizeHintForColumn(0) +
                      completer->popup()->verticalScrollBar()->sizeHint().width());

        completer->complete(rect);
   }
}

void CodeEditor::insertCompletion(const QString& txt)
{
    QTextCursor cursor = textCursor();

    cursor.insertText(txt.right(txt.length() - completer->completionPrefix().length()));
    setTextCursor(cursor);
}

void CodeEditor::toggleComment()
{
    QTextCursor cursor = textCursor();

    int end = document()->findBlock(cursor.selectionEnd()).blockNumber();

    cursor.beginEditBlock();

    for (int i =  document()->findBlock(cursor.selectionStart()).blockNumber();
             i <= end; i++) {

        QTextBlock block = document()->findBlockByNumber(i);

        cursor.setPosition(block.position(), QTextCursor::MoveAnchor);

        if (block.text()[0] == ';')
            cursor.deleteChar();
        else
            cursor.insertText(";");
    }

    cursor.endEditBlock();
}

QTextBlock CodeEditor::findBlockByY(int y)
{
    QTextBlock block = firstVisibleBlock();

    do {
        if (blockBoundingGeometry(block).contains(0, y))
            return block;

    } while ((block = block.next()).isValid());

    return QTextBlock();
}

void CodeEditor::ensureCursorVisible()
{
    QTextCursor cursor = textCursor();

    QTextBlock block = document()->findBlock(cursor.position());

    if (!block.isVisible()) {

        while (!(block = block.previous()).isVisible());

        cursor.setPosition(block.position(), QTextCursor::MoveAnchor);
        setTextCursor(cursor);
    }
}

QString CodeEditor::wordHelp()
{
    QString txt = textCursor().block().text();

    int i = textCursor().positionInBlock();

    while (++i < txt.length() && !(QChar(txt[i]).isSpace() || txt[i] == ')'));

    return txt.mid(textCursor().positionInBlock(), i - textCursor().positionInBlock());
}

bool CodeEditor::search(QString str, int flags)
{
    QTextCursor cursor = textCursor();

    cursor = (flags & RegExp) ? document()->find(QRegExp(str), cursor, static_cast<QTextDocument::FindFlag>(flags)) :
                                document()->find(str, cursor, static_cast<QTextDocument::FindFlag>(flags));
    if (cursor.isNull())
        return false;

    if (!cursor.block().isVisible())
        foldUnfoldAll(false);

    setTextCursor(cursor);
    return true;
}

bool CodeEditor::replace(QString str1, QString str2, int flags)
{
    QTextCursor cursor = textCursor();

    if (!cursor.hasSelection()) {
        return search(str1, flags);
    } else {
        int pos = textCursor().position() - textCursor().selectedText().length();

        cursor.beginEditBlock();
        cursor.insertText(str2);
        cursor.endEditBlock();

        if (flags & Backward) {
            cursor.setPosition(pos, QTextCursor::MoveAnchor);
            setTextCursor(cursor);
        }

        return true;
    }
}

void CodeEditor::replaceAll(QString str1, QString str2, int flags)
{
    QTextCursor cursor = textCursor();

    cursor.setPosition(0, QTextCursor::MoveAnchor);
    setTextCursor(cursor);

    while (replace(str1, str2, flags)) 1;
}

void CodeEditor::reconfig(int receiver)
{
    if (!(receiver & Config::Editor))
        return;

    QTextCharFormat fmt = config->colorScheme["Text"];

    QFont font(config->fontFamily, config->fontSize);
    QToolTip::setFont(font);
    //font.setFixedPitch(true);
    font.setBold(fmt.font().bold());
    font.setItalic(fmt.font().italic());
    setFont(font);

    QPalette palette;
    palette.setColor(QPalette::Text, fmt.foreground().color());
    palette.setColor(QPalette::Base, fmt.background().color());
    palette.setColor(QPalette::Highlight, Qt::yellow);
    palette.setColor(QPalette::HighlightedText, Qt::blue);
    setPalette(palette);

    fmt = config->colorScheme["Line Numbers"];

    font.setBold(fmt.font().bold());
    font.setItalic(fmt.font().italic());
    extraArea->setFont(font);

    palette.setColor(QPalette::Foreground, fmt.foreground().color());
    palette.setColor(QPalette::Background, fmt.background().color());
    extraArea->setPalette(palette);

    foldBoxWidth  = config->fontSize;
    foldBoxIndent = foldBoxWidth / 2;
    foldBoxLength = foldBoxWidth / 6;

    setTabStopWidth(FONTWIDTH * config->tabSize);

    if (!(receiver & Config::Init)) {

        QTextBlock block = document()->firstBlock();

        do { // receiver & Config::Highlighter
            block.setUserState(block.userState() | Rehighligh);
        } while ((block = block.next()).isValid());

        FULLRESIZE;
    }
}

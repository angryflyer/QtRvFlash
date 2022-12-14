/****************************************************************************
**
** Copyright (C) 2012 Denis Shienkov <denis.shienkov@gmail.com>
** Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtSerialPort module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "console.h"

#include <QScrollBar>

#include <QApplication>

Console::Console(QWidget *parent) :
    QPlainTextEdit(parent)
{
//    QFont qf;

//    qf.setPointSize(12);
//    setFont(qf);

//    document()->setMaximumBlockCount(100);
//    QPalette p = palette();
//    p.setColor(QPalette::Base, Qt::black);
//    p.setColor(QPalette::Text, Qt::green);
//    setPalette(p);
}

void Console::putData(const QByteArray &data)
{
    insertPlainText((QString)data);

    QScrollBar *bar = verticalScrollBar();
    bar->setValue(bar->maximum());
}

void Console::setLocalEchoEnabled(bool set)
{
    m_localEchoEnabled = set;
}

int Console::LOGI(const char *format,...)
{
    char buf[1000]; int i; //1000??????????????????????????????????????????????????????2000????????????
    va_list vlist;
    va_start(vlist,format);
    i=vsprintf(buf,format,vlist);
    va_end(vlist);
    insertPlainText(QString(buf));

    QScrollBar *bar = verticalScrollBar();
    bar->setValue(bar->maximum());

    QApplication::processEvents();
    return i;
}

void Console::keyPressEvent(QKeyEvent *e)
{
    switch (e->key()) {
    case Qt::Key_Backspace: {
        if(textCursor().columnNumber() > 2)
        textCursor().deletePreviousChar();
    }
        break;
    case Qt::Key_Left: {
        if(textCursor().columnNumber() > 2)
        moveCursor(QTextCursor::Left,QTextCursor::MoveAnchor);
    }
        break;
    case Qt::Key_Right: {
        moveCursor(QTextCursor::Right,QTextCursor::MoveAnchor);
    }
        break;
    case Qt::Key_Up: {
        moveCursor(QTextCursor::End,QTextCursor::MoveAnchor);
        while(textCursor().columnNumber() > 2) {
            textCursor().deletePreviousChar();
        }
        insertPlainText(QString(cmd_buffer[cmd_record_index]));
        if(cmd_record_index > 0) {
            cmd_record_index = cmd_record_index - 1;
        }
        cmd_status = 1;
    }
        break;
    case Qt::Key_Down: {
        moveCursor(QTextCursor::End,QTextCursor::MoveAnchor);
        while((textCursor().columnNumber() > 2)) {
            textCursor().deletePreviousChar();
        }
        if(cmd_record_index <= cmd_record_index_max) {
            insertPlainText(QString(cmd_buffer[cmd_record_index]));
            cmd_record_index = cmd_record_index + 1;
        } else {
            cmd_record_index = cmd_record_index_max;
            if(textCursor().columnNumber() > 2) {
                textCursor().deletePreviousChar();
            }
        }
        cmd_status = 2;
    }
        break;
    default:
        if(*(e->text().toLocal8Bit().data()) == '\r') {
            moveCursor(QTextCursor::End,QTextCursor::MoveAnchor);
        }
        if (m_localEchoEnabled)
            QPlainTextEdit::keyPressEvent(e);
        emit getData(e->text().toLocal8Bit());
    }
}

//void Console::mousePressEvent(QMouseEvent *e)
//{
//    Q_UNUSED(e)
//    setFocus();
//}

//void Console::mouseDoubleClickEvent(QMouseEvent *e)
//{
//    Q_UNUSED(e)
//}

//void Console::contextMenuEvent(QContextMenuEvent *e)
//{
//    Q_UNUSED(e)
//}

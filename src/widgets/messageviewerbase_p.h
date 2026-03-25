// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "messageviewer.h"

#include <QList>

class QAction;
class QMenuBar;
class QPrinter;
class QToolBar;
class QWidget;

class MessageViewerBasePrivate
{
public:
    explicit MessageViewerBasePrivate(QWidget *qq)
        : q(qq)
    {
    }

    QWidget *const q;
    int currentIndex = 0;
    QList<std::shared_ptr<KMime::Message>> messages;
    QString fileName;
    MimeTreeParser::Widgets::MessageViewer *messageViewer = nullptr;
    QAction *nextAction = nullptr;
    QAction *previousAction = nullptr;
    QToolBar *toolBar = nullptr;

    void save(QWidget *parent);
    void saveDecrypted(QWidget *parent);
    void print(QWidget *parent);
    void printPreview(QWidget *parent);
    void printInternal(QPrinter *printer);
};

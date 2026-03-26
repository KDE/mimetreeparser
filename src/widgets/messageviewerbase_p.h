// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QList>

#include <memory>

class KMessageWidget;
class QAction;
class QMenuBar;
class QPrinter;
class QStackedWidget;
class QStatusBar;
class QToolBar;
class QWidget;

namespace KMime
{
class Message;
}

namespace MimeTreeParser::Widgets
{
class MessageViewer;
}

class MessageViewerBasePrivate
{
public:
    explicit MessageViewerBasePrivate(QWidget *qq);

    QWidget *const q;
    int currentIndex = 0;
    QList<std::shared_ptr<KMime::Message>> messages;
    QString fileName;
    QStackedWidget *centralWidget = nullptr;
    MimeTreeParser::Widgets::MessageViewer *messageViewer = nullptr;
    KMessageWidget *errorMessage = nullptr;
    QAction *saveAction = nullptr;
    QAction *saveDecryptedAction = nullptr;
    QAction *printPreviewAction = nullptr;
    QAction *printAction = nullptr;
    QAction *nextAction = nullptr;
    QAction *previousAction = nullptr;
    QToolBar *toolBar = nullptr;
    QStatusBar *statusBar = nullptr;

    void setCurrentIndex(int currentIndex);

    void createToolBar(QWidget *parent);

private:
    void createActions(QWidget *parent);
    void createStatusBar(QWidget *parent);

    void save(QWidget *parent);
    void saveDecrypted(QWidget *parent);
    void print(QWidget *parent);
    void printPreview(QWidget *parent);
    void printInternal(QPrinter *printer);
};

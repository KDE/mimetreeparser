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
    static constexpr int CURRENT_INDEX_NOT_INITIALIZED = -2;
    static constexpr int CURRENT_INDEX_NO_MESSAGES = -1;

    QWidget *const q;

protected:
    explicit MessageViewerBasePrivate(QWidget *qq);

public:
    inline QList<std::shared_ptr<KMime::Message>> getMessages() const
    {
        return messages;
    }

    // return true if the messages were actually changed
    bool setMessages(const QList<std::shared_ptr<KMime::Message>> &messages);

public:
    QString fileName;
    // the central widget is created by the constructor
    QStackedWidget *centralWidget = nullptr;
    // the message viewer is created by the constructor
    MimeTreeParser::Widgets::MessageViewer *messageViewer = nullptr;
    // all actions are created by the constructor; they are disabled by default
    QAction *saveAction = nullptr;
    QAction *saveDecryptedAction = nullptr;
    QAction *printPreviewAction = nullptr;
    QAction *printAction = nullptr;
    QAction *nextAction = nullptr;
    QAction *previousAction = nullptr;
    // the tool bar is created by the constructor
    QToolBar *toolBar = nullptr;
    // the status bar is created by the constructor only if needed, i.e. it can be nullptr
    QStatusBar *statusBar = nullptr;

private:
    std::shared_ptr<KMime::Message> currentMessage() const;

    void setCurrentIndex(int currentIndex);
    void updateUI();

    void createActions(QWidget *parent);
    void createToolBar(QWidget *parent);
    void createStatusBar(QWidget *parent);

    void save(QWidget *parent);
    void saveDecrypted(QWidget *parent);
    void print(QWidget *parent);
    void printPreview(QWidget *parent);
    void printInternal(QPrinter *printer);

private:
    int currentIndex = CURRENT_INDEX_NOT_INITIALIZED;
    QList<std::shared_ptr<KMime::Message>> messages;
    KMessageWidget *errorMessage = nullptr;
};

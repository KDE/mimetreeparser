// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "messageviewerwindow.h"

#include "messageviewerbase_p.h"

#include <KLocalizedString>
#include <KMessageWidget>

#include <QMenuBar>
#include <QStackedWidget>
#include <QToolBar>

using namespace MimeTreeParser::Widgets;
using namespace Qt::StringLiterals;

class MessageViewerWindow::Private : public MessageViewerBasePrivate
{
public:
    explicit Private(MessageViewerWindow *window)
        : MessageViewerBasePrivate{window}
    {
    }

    QMenuBar *createMenuBar(QWidget *parent);
};

QMenuBar *MessageViewerWindow::Private::createMenuBar(QWidget *parent)
{
    const auto menuBar = new QMenuBar(parent);

    // File menu
    const auto fileMenu = menuBar->addMenu(i18nc("@action:inmenu", "&File"));
    fileMenu->addAction(saveAction);
    fileMenu->addAction(saveDecryptedAction);
    fileMenu->addAction(printPreviewAction);
    fileMenu->addAction(printAction);

    // Message menu
    const auto messageMenu = menuBar->addMenu(i18nc("@action:inmenu", "&Message"));
    messageMenu->setObjectName("messageMenu"); // gpgol.js relies on this. Don't remove!
    messageMenu->addAction(viewSourceAction);
    messageMenu->addAction(useFixedFontAction);
    messageMenu->addAction(previousAction);
    messageMenu->addAction(nextAction);

    return menuBar;
}

MessageViewerWindow::MessageViewerWindow(QWidget *parent)
    : QMainWindow(parent)
    , d(std::make_unique<Private>(this))
{
    initGUI();
}

void MessageViewerWindow::initGUI()
{
    const auto menuBar = d->createMenuBar(this);
    setMenuBar(menuBar);

    addToolBar(Qt::TopToolBarArea, d->toolBar);
    d->toolBar->setFloatable(false);
    d->toolBar->setMovable(false);

    setCentralWidget(d->centralWidget);

    setStatusBar(d->statusBar);

    setMinimumSize(300, 300);
    resize(600, 600);
}

MessageViewerWindow::~MessageViewerWindow() = default;

QToolBar *MessageViewerWindow::toolBar() const
{
    return d->toolBar;
}

QList<std::shared_ptr<KMime::Message>> MessageViewerWindow::messages() const
{
    return d->getMessages();
}

void MessageViewerWindow::setMessages(const QList<std::shared_ptr<KMime::Message>> &messages)
{
    if (d->setMessages(messages)) {
        Q_EMIT messagesChanged();
    }
}

#include "moc_messageviewerwindow.cpp"

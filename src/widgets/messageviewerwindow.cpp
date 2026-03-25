// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "messageviewerwindow.h"

#include "messageviewerbase_p.h"

#include <KLocalizedString>

#include <QDialog>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QToolBar>
#include <QVBoxLayout>

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
    void updateUI();
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

    auto viewRawAction = new QAction(QIcon::fromTheme(u"format-text-code-symbolic"_s), i18nc("@action:button", "View Source"));
    QObject::connect(viewRawAction, &QAction::triggered, parent, [this, parent]() {
        const auto message = messageViewer->message();
        const auto content = message->encodedContent();

        auto dialog = new QDialog(parent);
        auto layout = new QVBoxLayout(dialog);
        layout->setContentsMargins({});
        auto plainTextEdit = new QPlainTextEdit;
        plainTextEdit->setReadOnly(true);
        plainTextEdit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        plainTextEdit->appendPlainText(QString::fromUtf8(content));
        layout->addWidget(plainTextEdit);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        QFontMetrics metrics(plainTextEdit->font());
        dialog->setMinimumSize(80 * metrics.averageCharWidth() + 20, 500);
        dialog->show();
    });
    messageMenu->addAction(viewRawAction);

    auto fixedFontAction = new QAction(i18nc("@action:button", "Use Fixed Font"), parent);
    fixedFontAction->setCheckable(true);
    QObject::connect(fixedFontAction, &QAction::toggled, parent, [this](bool checked) {
        messageViewer->setFixedFont(checked);
    });
    messageMenu->addAction(fixedFontAction);

    messageMenu->addAction(previousAction);
    messageMenu->addAction(nextAction);

    return menuBar;
}

void MessageViewerWindow::Private::updateUI()
{
    const bool multipleMessages = messages.length() > 1;
    toolBar->setVisible(multipleMessages);
    nextAction->setEnabled(multipleMessages);
    nextAction->setVisible(multipleMessages);
    previousAction->setEnabled(multipleMessages);
    previousAction->setVisible(multipleMessages);

    if (!messages.isEmpty()) {
        setCurrentIndex(0);
    } else {
        currentIndex = 0;
    }
}

MessageViewerWindow::MessageViewerWindow(QWidget *parent)
    : QMainWindow(parent)
    , d(std::make_unique<Private>(this))
{
    initGUI();
    connect(this, &MessageViewerWindow::messagesChanged, this, [this]() {
        d->updateUI();
    });
}

void MessageViewerWindow::initGUI()
{
    const auto menuBar = d->createMenuBar(this);
    setMenuBar(menuBar);

    d->createToolBar(this);

    d->toolBar->hide();
    addToolBar(Qt::TopToolBarArea, d->toolBar);
    d->toolBar->setFloatable(false);
    d->toolBar->setMovable(false);

    d->messageViewer = new MimeTreeParser::Widgets::MessageViewer(this);
    setWindowTitle(d->messageViewer->subject());
    setCentralWidget(d->messageViewer);

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
    return d->messages;
}

void MessageViewerWindow::setMessages(const QList<std::shared_ptr<KMime::Message>> &messages)
{
    if (d->messages == messages) {
        return;
    }
    d->messages = messages;
    Q_EMIT messagesChanged();
}

#include "moc_messageviewerwindow.cpp"

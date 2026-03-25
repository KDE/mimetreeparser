// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "messageviewerwindow.h"

#include "messageviewerbase_p.h"

#include <KColorScheme>
#include <KLocalizedString>

#include <Libkleo/Compliance>

#include <QDialog>
#include <QLabel>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QStatusBar>
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

    void setCurrentIndex(int currentIndex);
    QMenuBar *createMenuBar(QWidget *parent);
    void updateUI();
};

void MessageViewerWindow::Private::setCurrentIndex(int index)
{
    Q_ASSERT(index >= 0);
    Q_ASSERT(index < messages.count());

    currentIndex = index;
    messageViewer->setMessage(messages[currentIndex]);
    q->setWindowTitle(messageViewer->subject());

    previousAction->setEnabled(currentIndex != 0);
    nextAction->setEnabled(currentIndex != messages.count() - 1);

    auto subject = messages[currentIndex]->subject()->asUnicodeString();
    q->setWindowTitle(subject.isEmpty() ? i18nc("window title if email subject is empty", "(No Subject)") : subject);
}

QMenuBar *MessageViewerWindow::Private::createMenuBar(QWidget *parent)
{
    const auto menuBar = new QMenuBar(parent);

    // File menu
    const auto fileMenu = menuBar->addMenu(i18nc("@action:inmenu", "&File"));

    const auto saveAction = new QAction(QIcon::fromTheme(u"document-save"_s), i18nc("@action:inmenu", "&Save"));
    QObject::connect(saveAction, &QAction::triggered, parent, [parent, this] {
        save(parent);
    });
    fileMenu->addAction(saveAction);

    const auto saveDecryptedAction = new QAction(QIcon::fromTheme(u"document-save"_s), i18nc("@action:inmenu", "Save Decrypted"));
    QObject::connect(saveDecryptedAction, &QAction::triggered, parent, [parent, this] {
        saveDecrypted(parent);
    });
    fileMenu->addAction(saveDecryptedAction);

    const auto printPreviewAction = new QAction(QIcon::fromTheme(u"document-print-preview"_s), i18nc("@action:inmenu", "Print Preview"));
    QObject::connect(printPreviewAction, &QAction::triggered, parent, [parent, this] {
        printPreview(parent);
    });
    fileMenu->addAction(printPreviewAction);

    const auto printAction = new QAction(QIcon::fromTheme(u"document-print"_s), i18nc("@action:inmenu", "&Print"));
    QObject::connect(printAction, &QAction::triggered, parent, [parent, this] {
        print(parent);
    });
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

    previousAction = new QAction(QIcon::fromTheme(u"go-previous"_s), i18nc("@action:button Previous email", "Previous Message"), parent);
    previousAction->setEnabled(false);
    messageMenu->addAction(previousAction);

    nextAction = new QAction(QIcon::fromTheme(u"go-next"_s), i18nc("@action:button Next email", "Next Message"), parent);
    nextAction->setEnabled(false);
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
    currentIndex = 0;

    if (messages.length() > 0) {
        messageViewer->setMessage(messages[0]);

        auto subject = messages[currentIndex]->subject()->asUnicodeString();
        q->setWindowTitle(subject.isEmpty() ? i18nc("window title if email subject is empty", "(No Subject)") : subject);
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

    d->toolBar = new QToolBar(this);

#ifdef Q_OS_UNIX
    d->toolBar->setToolButtonStyle(Qt::ToolButtonFollowStyle);
#else
    // on other platforms the default is IconOnly which is bad for
    // accessibility and can't be changed by the user.
    d->toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
#endif

    d->toolBar->addAction(d->previousAction);
    connect(d->previousAction, &QAction::triggered, this, [this] {
        d->setCurrentIndex(d->currentIndex - 1);
    });

    const auto spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    d->toolBar->addWidget(spacer);

    d->toolBar->addAction(d->nextAction);
    connect(d->nextAction, &QAction::triggered, this, [this] {
        d->setCurrentIndex(d->currentIndex + 1);
    });

    d->toolBar->hide();
    addToolBar(Qt::TopToolBarArea, d->toolBar);
    d->toolBar->setFloatable(false);
    d->toolBar->setMovable(false);

    d->messageViewer = new MimeTreeParser::Widgets::MessageViewer(this);
    setWindowTitle(d->messageViewer->subject());
    setCentralWidget(d->messageViewer);

    if (Kleo::DeVSCompliance::isActive()) {
        auto statusBar = new QStatusBar(this);
        auto statusLbl = std::make_unique<QLabel>(Kleo::DeVSCompliance::name());
        {
            auto statusPalette = qApp->palette();
            KColorScheme::adjustForeground(statusPalette,
                                           Kleo::DeVSCompliance::isCompliant() ? KColorScheme::NormalText : KColorScheme::NegativeText,
                                           statusLbl->foregroundRole(),
                                           KColorScheme::View);
            statusLbl->setAutoFillBackground(true);
            KColorScheme::adjustBackground(statusPalette,
                                           Kleo::DeVSCompliance::isCompliant() ? KColorScheme::PositiveBackground : KColorScheme::NegativeBackground,
                                           QPalette::Window,
                                           KColorScheme::View);
            statusLbl->setPalette(statusPalette);
        }
        statusBar->addPermanentWidget(statusLbl.release());
        setStatusBar(statusBar);
    }

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

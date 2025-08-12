// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "messageviewerwindow.h"
using namespace Qt::Literals::StringLiterals;

#include "messageviewer.h"
#include "messageviewerutils_p.h"
#include <MimeTreeParserCore/CryptoHelper>
#include <MimeTreeParserCore/FileOpener>

#include <KColorScheme>
#include <KLocalizedString>
#include <KMessageBox>
#include <KMessageWidget>
#include <Libkleo/Compliance>

#include <QFileDialog>
#include <QFontMetrics>
#include <QLabel>
#include <QMenuBar>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QPushButton>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QStatusBar>
#include <QStyle>
#include <QToolBar>
#include <QVBoxLayout>

using namespace MimeTreeParser::Widgets;
using namespace Qt::StringLiterals;

class MessageViewerWindow::Private
{
public:
    Private(MessageViewerWindow *window)
        : q(window)
    {
    }

    MessageViewerWindow *const q;
    int currentIndex = 0;
    QList<KMime::Message::Ptr> messages;
    QString fileName;
    MimeTreeParser::Widgets::MessageViewer *messageViewer = nullptr;
    QAction *nextAction = nullptr;
    QAction *previousAction = nullptr;
    QToolBar *toolBar = nullptr;

    void setCurrentIndex(int currentIndex);
    QMenuBar *createMenuBar(QWidget *parent);
    void updateUI();

private:
    void save(QWidget *parent);
    void saveDecrypted(QWidget *parent);
    void print(QWidget *parent);
    void printPreview(QWidget *parent);
    void printInternal(QPrinter *printer);
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

void MessageViewerWindow::Private::save(QWidget *parent)
{
    QString extension;
    QString alternatives;
    auto message = messages[currentIndex];
    bool wasEncrypted = false;
    GpgME::Protocol protocol;
    auto decryptedMessage = CryptoUtils::decryptMessage(message, wasEncrypted, protocol);
    Q_UNUSED(decryptedMessage); // we save the message without modifying it

    if (wasEncrypted) {
        extension = u".mime"_s;
        if (protocol == GpgME::OpenPGP) {
            alternatives = i18nc("File dialog accepted files", "Email files (*.eml *.mbox *.mime)");
        } else {
            alternatives = i18nc("File dialog accepted files", "Encrypted S/MIME files (*.p7m)");
        }
    } else {
        extension = u".eml"_s;
        alternatives = i18nc("Accepted files in a file dialog. You only need to translate 'file'", "EML file (*.eml);;MBOX file (*.mbox);;MIME file (*.mime)");
    }

    const QString location =
        QFileDialog::getSaveFileName(parent,
                                     i18nc("@title:window", "Save File"),
                                     MesageViewerUtils::changeExtension(MesageViewerUtils::changeFileName(fileName, messageViewer->subject()), extension),
                                     alternatives);

    QSaveFile file(location);
    if (!file.open(QIODevice::WriteOnly)) {
        KMessageBox::error(parent, i18n("File %1 could not be created.", location), i18nc("@title:window", "Error saving message"));
        return;
    }
    file.write(messages[currentIndex]->encodedContent());
    file.commit();
}

void MessageViewerWindow::Private::saveDecrypted(QWidget *parent)
{
    const QString location =
        QFileDialog::getSaveFileName(parent,
                                     i18nc("@title:window", "Save Decrypted File"),
                                     MesageViewerUtils::changeExtension(MesageViewerUtils::changeFileName(fileName, messageViewer->subject()), u".eml"_s),
                                     i18nc("File dialog accepted files", "Email files (*.eml *.mbox *.mime)"));

    QSaveFile file(location);
    if (!file.open(QIODevice::WriteOnly)) {
        KMessageBox::error(parent, i18nc("Error message", "File %1 could not be created.", location), i18nc("@title:window", "Error saving message"));
        return;
    }
    auto message = messages[currentIndex];
    bool wasEncrypted = false;
    GpgME::Protocol protocol;
    auto decryptedMessage = CryptoUtils::decryptMessage(message, wasEncrypted, protocol);
    if (!wasEncrypted) {
        decryptedMessage = message;
    }
    file.write(decryptedMessage->encodedContent());

    file.commit();
}

void MessageViewerWindow::Private::print(QWidget *parent)
{
    QPrinter printer;
    QPrintDialog dialog(&printer, parent);
    dialog.setWindowTitle(i18nc("@title:window", "Print"));
    if (dialog.exec() != QDialog::Accepted)
        return;

    printInternal(&printer);
}

void MessageViewerWindow::Private::printPreview(QWidget *parent)
{
    auto dialog = new QPrintPreviewDialog(parent);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->resize(800, 750);
    dialog->setWindowTitle(i18nc("@title:window", "Print Preview"));
    QObject::connect(dialog, &QPrintPreviewDialog::paintRequested, parent, [this](QPrinter *printer) {
        printInternal(printer);
    });
    dialog->open();
}

void MessageViewerWindow::Private::printInternal(QPrinter *printer)
{
    QPainter painter;
    painter.begin(printer);
    const auto pageLayout = printer->pageLayout();
    const auto pageRect = pageLayout.paintRectPixels(printer->resolution());
    const double xscale = pageRect.width() / double(messageViewer->width());
    const double yscale = pageRect.height() / double(messageViewer->height());
    const double scale = qMin(qMin(xscale, yscale), 1.);
    painter.translate(pageRect.x(), pageRect.y());
    painter.scale(scale, scale);
    messageViewer->print(&painter, pageRect.width());
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

QList<KMime::Message::Ptr> MessageViewerWindow::messages() const
{
    return d->messages;
}

void MessageViewerWindow::setMessages(const QList<KMime::Message::Ptr> &messages)
{
    if (d->messages == messages) {
        return;
    }
    d->messages = messages;
    Q_EMIT messagesChanged();
}

#include "moc_messageviewerwindow.cpp"

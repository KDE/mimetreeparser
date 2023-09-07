// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "messageviewerdialog.h"

#include "messageviewer.h"
#include "mimetreeparser_widgets_debug.h"

#include <KLocalizedString>
#include <KMessageWidget>
#include <KMime/Message>

#include <QDialogButtonBox>
#include <QFile>
#include <QMenuBar>
#include <QMimeDatabase>
#include <QMimeType>
#include <QPainter>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QPushButton>
#include <QStyle>
#include <QToolBar>
#include <QVBoxLayout>

#include <memory>

using namespace MimeTreeParser::Widgets;

namespace
{

/// Open first message from file
QVector<KMime::Message::Ptr> openFile(const QString &fileName)
{
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForFile(fileName);
    KMime::Message::Ptr message(new KMime::Message);

    QFile file(fileName);
    file.open(QIODevice::ReadOnly);
    if (!file.isOpen()) {
        qCWarning(MIMETREEPARSER_WIDGET_LOG) << "Could not open file";
        return {};
    }

    const auto content = file.readAll();

    if (content.length() == 0) {
        qCWarning(MIMETREEPARSER_WIDGET_LOG) << "File is empty";
        return {};
    }

    if (mime.inherits(QStringLiteral("application/pgp-encrypted")) || fileName.endsWith(QStringLiteral(".asc"))) {
        auto contentType = message->contentType();
        contentType->setMimeType("multipart/encrypted");
        contentType->setBoundary(KMime::multiPartBoundary());
        contentType->setParameter(QStringLiteral("protocol"), QStringLiteral("application/pgp-encrypted"));
        contentType->setCategory(KMime::Headers::CCcontainer);

        auto cte = message->contentTransferEncoding();
        cte->setEncoding(KMime::Headers::CE7Bit);
        cte->setDecoded(true);

        auto pgpEncrypted = new KMime::Content;
        pgpEncrypted->contentType()->setMimeType("application/pgp-encrypted");
        auto contentDisposition = new KMime::Headers::ContentDisposition;
        contentDisposition->setDisposition(KMime::Headers::CDattachment);
        pgpEncrypted->appendHeader(contentDisposition);
        pgpEncrypted->setBody("Version: 1");
        message->addContent(pgpEncrypted);

        auto encryptedContent = new KMime::Content;
        encryptedContent->contentType()->setMimeType("application/octet-stream");
        contentDisposition = new KMime::Headers::ContentDisposition;
        contentDisposition->setDisposition(KMime::Headers::CDinline);
        contentDisposition->setFilename(QStringLiteral("msg.asc"));
        encryptedContent->appendHeader(contentDisposition);
        encryptedContent->setBody(content);
        message->addContent(encryptedContent);

        message->assemble();
    } else {
        int startOfMessage = 0;
        if (content.startsWith("From ")) {
            startOfMessage = content.indexOf('\n');
            if (startOfMessage == -1) {
                return {};
            }
            startOfMessage += 1; // the message starts after the '\n'
        }
        QVector<KMime::Message::Ptr> listMessages;

        // check for multiple messages in the file
        int endOfMessage = content.indexOf("\nFrom ", startOfMessage);
        while (endOfMessage != -1) {
            if (content.indexOf("From ", startOfMessage) == startOfMessage) {
                startOfMessage = content.indexOf('\n', startOfMessage) + 1;
            }
            auto msg = new KMime::Message;
            msg->setContent(KMime::CRLFtoLF(content.mid(startOfMessage, endOfMessage - startOfMessage)));
            msg->parse();
            if (!msg->hasContent()) {
                delete msg;
                msg = nullptr;
                return {};
            }
            KMime::Message::Ptr mMsg(msg);
            listMessages << mMsg;
            startOfMessage = endOfMessage + 1;
            endOfMessage = content.indexOf("\nFrom ", startOfMessage);
        }
        if (endOfMessage == -1) {
            if (content.indexOf("From ", startOfMessage) == startOfMessage) {
                startOfMessage = content.indexOf('\n', startOfMessage) + 1;
            }
            endOfMessage = content.length();
            auto msg = new KMime::Message;
            msg->setContent(KMime::CRLFtoLF(content.mid(startOfMessage, endOfMessage - startOfMessage)));
            msg->parse();
            if (!msg->hasContent()) {
                delete msg;
                msg = nullptr;
                return {};
            }
            KMime::Message::Ptr mMsg(msg);
            listMessages << mMsg;
        }
        return listMessages;
    }

    return {message};
}
}

class MessageViewerDialog::Private
{
public:
    int currentIndex = 0;
    QVector<KMime::Message::Ptr> messages;
    MimeTreeParser::Widgets::MessageViewer *messageViewer = nullptr;
    QAction *nextAction = nullptr;
    QAction *previousAction = nullptr;

    void setCurrentIndex(int currentIndex);
    QMenuBar *createMenuBar(QWidget *parent);

private:
    void print(QWidget *parent);
    void printPreview(QWidget *parent);
    void printInternal(QPrinter *printer);
};

void MessageViewerDialog::Private::setCurrentIndex(int index)
{
    Q_ASSERT(index >= 0);
    Q_ASSERT(index < messages.count());

    currentIndex = index;
    messageViewer->setMessage(messages[currentIndex]);

    previousAction->setEnabled(currentIndex != 0);
    nextAction->setEnabled(currentIndex != messages.count() - 1);
}

QMenuBar *MessageViewerDialog::Private::createMenuBar(QWidget *parent)
{
    const auto menuBar = new QMenuBar(parent);

    // File menu
    const auto fileMenu = menuBar->addMenu(i18nc("@action:inmenu", "&File"));
    const auto saveAction = new QAction(QIcon::fromTheme(QStringLiteral("document-save")), i18nc("@action:inmenu", "&Save"));
    fileMenu->addAction(saveAction);

    const auto printPreviewAction = new QAction(QIcon::fromTheme(QStringLiteral("document-print-preview")), i18nc("@action:inmenu", "Print Preview"));
    QObject::connect(printPreviewAction, &QAction::triggered, parent, [parent, this] {
        printPreview(parent);
    });
    fileMenu->addAction(printPreviewAction);

    const auto printAction = new QAction(QIcon::fromTheme(QStringLiteral("document-print")), i18nc("@action:inmenu", "&Print"));
    QObject::connect(printAction, &QAction::triggered, parent, [parent, this] {
        print(parent);
    });
    fileMenu->addAction(printAction);

    // Navigation menu
    const auto navigationMenu = menuBar->addMenu(i18nc("@action:inmenu", "&Navigation"));
    previousAction = new QAction(QIcon::fromTheme(QStringLiteral("go-previous")), i18nc("@action:button Previous email", "Previous Message"), parent);
    previousAction->setEnabled(false);
    navigationMenu->addAction(previousAction);

    nextAction = new QAction(QIcon::fromTheme(QStringLiteral("go-next")), i18nc("@action:button Next email", "Next Message"), parent);
    nextAction->setEnabled(false);
    navigationMenu->addAction(nextAction);

    return menuBar;
}

void MessageViewerDialog::Private::print(QWidget *parent)
{
    QPrinter printer;
    QPrintDialog dialog(&printer, parent);
    dialog.setWindowTitle(i18nc("@title:window", "Print Document"));
    if (dialog.exec() != QDialog::Accepted)
        return;

    printInternal(&printer);
}

void MessageViewerDialog::Private::printPreview(QWidget *parent)
{
    auto dialog = new QPrintPreviewDialog(parent);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->resize(800, 750);
    dialog->setWindowTitle(i18nc("@title:window", "Print Document"));
    QObject::connect(dialog, &QPrintPreviewDialog::paintRequested, parent, [this](QPrinter *printer) {
        printInternal(printer);
    });
    dialog->open();
}

void MessageViewerDialog::Private::printInternal(QPrinter *printer)
{
    QPainter painter;
    painter.begin(printer);
    const auto pageLayout = printer->pageLayout();
    const auto pageRect = pageLayout.paintRectPixels(printer->resolution());
    const auto paperRect = pageLayout.fullRectPixels(printer->resolution());
    const double xscale = pageRect.width() / double(messageViewer->width());
    const double yscale = pageRect.height() / double(messageViewer->height());
    const double scale = qMin(qMin(xscale, yscale), 1.);
    painter.translate(pageRect.x(), pageRect.y());
    painter.scale(scale, scale);
    messageViewer->print(&painter, pageRect.width());
}

MessageViewerDialog::MessageViewerDialog(const QString &fileName, QWidget *parent)
    : QDialog(parent)
    , d(std::make_unique<Private>())
{
    const auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins({});

    const auto layout = new QVBoxLayout;
    layout->setContentsMargins(style()->pixelMetric(QStyle::PM_LayoutLeftMargin, nullptr, this),
                               style()->pixelMetric(QStyle::PM_LayoutTopMargin, nullptr, this),
                               style()->pixelMetric(QStyle::PM_LayoutRightMargin, nullptr, this),
                               style()->pixelMetric(QStyle::PM_LayoutBottomMargin, nullptr, this));

    const auto menuBar = d->createMenuBar(this);
    mainLayout->setMenuBar(menuBar);

    d->messages += openFile(fileName);

    if (d->messages.isEmpty()) {
        auto errorMessage = new KMessageWidget(this);
        errorMessage->setMessageType(KMessageWidget::Error);
        errorMessage->setText(i18nc("@info", "Unable to read file"));
        layout->addWidget(errorMessage);
        return;
    }

    const bool multipleMessages = d->messages.length() > 1;
    if (multipleMessages) {
        const auto toolBar = new QToolBar(this);
#ifdef Q_OS_UNIX
        toolBar->setToolButtonStyle(Qt::ToolButtonFollowStyle);
#else
        // on other platforms the default is IconOnly which is bad for
        // accessibility and can't be changed by the user.
        toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
#endif

        toolBar->addAction(d->previousAction);
        connect(d->previousAction, &QAction::triggered, this, [this](int index) {
            d->setCurrentIndex(d->currentIndex - 1);
        });

        const auto spacer = new QWidget(this);
        spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        toolBar->addWidget(spacer);

        toolBar->addAction(d->nextAction);
        connect(d->nextAction, &QAction::triggered, this, [this](int index) {
            d->setCurrentIndex(d->currentIndex + 1);
        });
        d->nextAction->setEnabled(true);

        mainLayout->addWidget(toolBar);
    }

    mainLayout->addLayout(layout);

    d->messageViewer = new MimeTreeParser::Widgets::MessageViewer(this);
    d->messageViewer->setMessage(d->messages[0]);
    layout->addWidget(d->messageViewer);

    auto buttonBox = new QDialogButtonBox(this);
    auto closeButton = buttonBox->addButton(QDialogButtonBox::Close);
    connect(closeButton, &QPushButton::pressed, this, &QDialog::accept);
    layout->addWidget(buttonBox);
}

MessageViewerDialog::~MessageViewerDialog() = default;

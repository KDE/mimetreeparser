// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "messageviewerbase_p.h"

#include "messageviewer.h"
#include "messageviewerutils_p.h"

#include <MimeTreeParserCore/CryptoHelper>

#include <Libkleo/Compliance>

#include <KColorScheme>
#include <KLocalizedString>
#include <KMessageBox>
#include <KMessageWidget>

#include <QApplication>
#include <QFileDialog>
#include <QLabel>
#include <QPainter>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QSaveFile>
#include <QStackedWidget>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>

#include <gpgme++/global.h>

#include <mimetreeparser_widgets_debug.h>

using namespace MimeTreeParser;
using namespace Qt::Literals::StringLiterals;

MessageViewerBasePrivate::MessageViewerBasePrivate(QWidget *qq)
    : q(qq)
{
    createActions(q);
    createToolBar(q);
    createStatusBar(q);
    centralWidget = new QStackedWidget(q);
    messageViewer = new MimeTreeParser::Widgets::MessageViewer(q);
    centralWidget->addWidget(messageViewer);
    auto errorPage = new QWidget(q);
    auto errorLayout = new QVBoxLayout(errorPage);
    errorLayout->addStretch();
    errorMessage = new KMessageWidget(q);
    errorMessage->setMessageType(KMessageWidget::Error);
    errorMessage->setCloseButtonVisible(false);
    errorMessage->setWordWrap(true);
    errorLayout->addWidget(errorMessage);
    errorLayout->addStretch();
    centralWidget->addWidget(errorPage);
}

void MessageViewerBasePrivate::createActions(QWidget *parent)
{
    saveAction = new QAction(QIcon::fromTheme(u"document-save"_s), i18nc("@action:inmenu", "&Save"), parent);
    QObject::connect(saveAction, &QAction::triggered, parent, [parent, this] {
        save(parent);
    });
    saveAction->setEnabled(false);

    saveDecryptedAction = new QAction(QIcon::fromTheme(u"document-save"_s), i18nc("@action:inmenu", "Save Decrypted"), parent);
    QObject::connect(saveDecryptedAction, &QAction::triggered, parent, [parent, this] {
        saveDecrypted(parent);
    });
    saveDecryptedAction->setEnabled(false);

    printPreviewAction = new QAction(QIcon::fromTheme(u"document-print-preview"_s), i18nc("@action:inmenu", "Print Preview"), parent);
    QObject::connect(printPreviewAction, &QAction::triggered, parent, [parent, this] {
        printPreview(parent);
    });
    printPreviewAction->setEnabled(false);

    printAction = new QAction(QIcon::fromTheme(u"document-print"_s), i18nc("@action:inmenu", "&Print"), parent);
    QObject::connect(printAction, &QAction::triggered, parent, [parent, this] {
        print(parent);
    });
    printAction->setEnabled(false);

    previousAction = new QAction(QIcon::fromTheme(u"go-previous"_s), i18nc("@action:button Previous email", "Previous Message"), parent);
    QObject::connect(previousAction, &QAction::triggered, parent, [this] {
        setCurrentIndex(currentIndex - 1);
    });
    previousAction->setEnabled(false);

    nextAction = new QAction(QIcon::fromTheme(u"go-next"_s), i18nc("@action:button Next email", "Next Message"), parent);
    QObject::connect(nextAction, &QAction::triggered, parent, [this] {
        setCurrentIndex(currentIndex + 1);
    });
    nextAction->setEnabled(false);
}

void MessageViewerBasePrivate::createStatusBar(QWidget *parent)
{
    if (Kleo::DeVSCompliance::isActive()) {
        statusBar = new QStatusBar(parent);
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
    }
}

std::shared_ptr<KMime::Message> MessageViewerBasePrivate::currentMessage() const
{
    return messages.empty() ? std::shared_ptr<KMime::Message>{} : messageViewer->message();
}

void MessageViewerBasePrivate::setCurrentIndex(int index)
{
    Q_ASSERT(index >= 0 || index == CURRENT_INDEX_NO_MESSAGES);
    Q_ASSERT(index < messages.size());

    if (index >= 0 && index < messages.size()) {
        currentIndex = index;
        messageViewer->setMessage(messages[currentIndex]);

        previousAction->setEnabled(currentIndex != 0);
        nextAction->setEnabled(currentIndex != messages.size() - 1);

        const QString subject = messageViewer->subject();
        q->setWindowTitle(subject.isEmpty() ? i18nc("window title if email subject is empty", "(No Subject)") : subject);
    } else if (index == CURRENT_INDEX_NO_MESSAGES) {
        currentIndex = index;

        previousAction->setEnabled(false);
        nextAction->setEnabled(false);

        q->setWindowTitle(i18nc("@title:window if there's no email to display", "(No Message)"));
    } else {
        qCWarning(MIMETREEPARSER_WIDGET_LOG) << __func__ << "called with invalid index" << index;
    }
}

void MessageViewerBasePrivate::updateUI()
{
    const bool hasMessages = !messages.empty();
    const bool hasMultipleMessages = messages.size() > 1;

    if (hasMessages) {
        centralWidget->setCurrentIndex(0);
    } else {
        if (fileName.isEmpty()) {
            errorMessage->setText(xi18nc("@info", "No messages to display."));
        } else {
            errorMessage->setText(xi18nc("@info", "Unable to read the file <filename>%1</filename>, or the file doesn't contain any messages.", fileName));
        }
        centralWidget->setCurrentIndex(1);
    }

    saveAction->setEnabled(hasMessages);
    saveDecryptedAction->setEnabled(hasMessages);
    printPreviewAction->setEnabled(hasMessages);
    printAction->setEnabled(hasMessages);

    toolBar->setVisible(hasMultipleMessages);
    nextAction->setVisible(hasMultipleMessages);
    previousAction->setVisible(hasMultipleMessages);
}

bool MessageViewerBasePrivate::setMessages(const QList<std::shared_ptr<KMime::Message>> &newMessages)
{
    if ((messages == newMessages) && currentIndex != CURRENT_INDEX_NOT_INITIALIZED) {
        return false;
    }
    messages = newMessages;

    updateUI();

    if (!messages.isEmpty()) {
        setCurrentIndex(0);
    } else {
        setCurrentIndex(CURRENT_INDEX_NO_MESSAGES);
    }

    return true;
}

void MessageViewerBasePrivate::createToolBar(QWidget *parent)
{
    toolBar = new QToolBar(parent);

#ifdef Q_OS_UNIX
    toolBar->setToolButtonStyle(Qt::ToolButtonFollowStyle);
#else
    // on other platforms the default is IconOnly which is bad for
    // accessibility and can't be changed by the user.
    toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
#endif

    toolBar->addAction(previousAction);

    const auto spacer = new QWidget(parent);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    toolBar->addWidget(spacer);

    toolBar->addAction(nextAction);
    toolBar->hide();
}

void MessageViewerBasePrivate::save(QWidget *parent)
{
    auto message = currentMessage();
    if (!message) {
        return;
    }

    QString extension;
    QString alternatives;
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
                                     MessageViewerUtils::changeExtension(MessageViewerUtils::changeFileName(fileName, messageViewer->subject()), extension),
                                     alternatives);

    QSaveFile file(location);
    if (!file.open(QIODevice::WriteOnly)) {
        KMessageBox::error(parent, i18n("File %1 could not be created.", location), i18nc("@title:window", "Error saving message"));
        return;
    }
    file.write(message->encodedContent());
    file.commit();
}

void MessageViewerBasePrivate::saveDecrypted(QWidget *parent)
{
    auto message = currentMessage();
    if (!message) {
        return;
    }

    const QString location =
        QFileDialog::getSaveFileName(parent,
                                     i18nc("@title:window", "Save Decrypted File"),
                                     MessageViewerUtils::changeExtension(MessageViewerUtils::changeFileName(fileName, messageViewer->subject()), u".eml"_s),
                                     i18nc("File dialog accepted files", "Email files (*.eml *.mbox *.mime)"));

    QSaveFile file(location);
    if (!file.open(QIODevice::WriteOnly)) {
        KMessageBox::error(parent, i18nc("Error message", "File %1 could not be created.", location), i18nc("@title:window", "Error saving message"));
        return;
    }
    bool wasEncrypted = false;
    GpgME::Protocol protocol;
    auto decryptedMessage = CryptoUtils::decryptMessage(message, wasEncrypted, protocol);
    if (!wasEncrypted) {
        decryptedMessage = message;
    }
    file.write(decryptedMessage->encodedContent());

    file.commit();
}

void MessageViewerBasePrivate::print(QWidget *parent)
{
    if (!currentMessage()) {
        return;
    }

    QPrinter printer;
    QPrintDialog dialog(&printer, parent);
    dialog.setWindowTitle(i18nc("@title:window", "Print"));
    if (dialog.exec() != QDialog::Accepted)
        return;

    printInternal(&printer);
}

void MessageViewerBasePrivate::printPreview(QWidget *parent)
{
    if (!currentMessage()) {
        return;
    }

    auto dialog = new QPrintPreviewDialog(parent);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->resize(800, 750);
    dialog->setWindowTitle(i18nc("@title:window", "Print Preview"));
    QObject::connect(dialog, &QPrintPreviewDialog::paintRequested, parent, [this](QPrinter *printer) {
        printInternal(printer);
    });
    dialog->open();
}

void MessageViewerBasePrivate::printInternal(QPrinter *printer)
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

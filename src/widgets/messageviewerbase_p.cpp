// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "messageviewerbase_p.h"

#include "messageviewerutils_p.h"

#include <MimeTreeParserCore/CryptoHelper>

#include <KLocalizedString>
#include <KMessageBox>

#include <QFileDialog>
#include <QPainter>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QSaveFile>

#include <gpgme++/global.h>

using namespace MimeTreeParser;
using namespace Qt::Literals::StringLiterals;

void MessageViewerBasePrivate::setCurrentIndex(int index)
{
    Q_ASSERT(index >= 0);
    Q_ASSERT(index < messages.count());

    if (index < 0 || index >= messages.size()) {
        return;
    }

    currentIndex = index;
    messageViewer->setMessage(messages[currentIndex]);

    previousAction->setEnabled(currentIndex != 0);
    nextAction->setEnabled(currentIndex != messages.count() - 1);

    const QString subject = messageViewer->subject();
    q->setWindowTitle(subject.isEmpty() ? i18nc("window title if email subject is empty", "(No Subject)") : subject);
}

void MessageViewerBasePrivate::save(QWidget *parent)
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

void MessageViewerBasePrivate::saveDecrypted(QWidget *parent)
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

void MessageViewerBasePrivate::print(QWidget *parent)
{
    QPrinter printer;
    QPrintDialog dialog(&printer, parent);
    dialog.setWindowTitle(i18nc("@title:window", "Print"));
    if (dialog.exec() != QDialog::Accepted)
        return;

    printInternal(&printer);
}

void MessageViewerBasePrivate::printPreview(QWidget *parent)
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

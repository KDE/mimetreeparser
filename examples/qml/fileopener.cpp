// SPDX-FileCopyrightText: 2023 Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "fileopener.h"
#include <KMbox/MBox>
#include <MimeTreeParser/ObjectTreeParser>
#include <QDebug>
#include <QFile>
#include <QMimeDatabase>
#include <QMimeType>

void FileOpener::open(const QUrl &url)
{
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForFile(url.fileName());

    qWarning() << mime;

    if (mime.inherits(QStringLiteral("application/mbox"))) {
        KMBox::MBox mbox;
        mbox.load(url.toLocalFile());
        const auto entries = mbox.entries();
        qWarning() << entries.count();
        KMime::Message::Ptr message(mbox.readMessage(entries[0]));
        Q_EMIT messageOpened(message);
        return;
    }

    QFile file(url.toLocalFile());
    file.open(QIODevice::ReadOnly);
    if (!file.isOpen()) {
        qWarning() << "Could not open file";
        return;
    }

    const auto content = file.readAll();

    if (content.length() == 0) {
        qWarning() << "File is empty";
        return;
    }
    KMime::Message::Ptr message(new KMime::Message);
    message->fromUnicodeString(QString::fromUtf8(content));
    Q_EMIT messageOpened(message);
}

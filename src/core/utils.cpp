// SPDX-FileCopyrightText: 2016 Sandro Knauß <sknauss@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "utils.h"

#include <KLocalizedString>

using namespace MimeTreeParser;

KMime::Content *MimeTreeParser::findTypeInDirectChildren(KMime::Content *content, const QByteArray &mimeType)
{
    const auto contents = content->contents();
    for (const auto child : contents) {
        if ((!child->contentType()->isEmpty()) && (mimeType == child->contentType()->mimeType())) {
            return child;
        }
    }
    return nullptr;
}

QString MimeTreeParser::decryptRecipientsToHtml(const std::vector<std::pair<GpgME::DecryptionResult::Recipient, GpgME::Key>> &recipients,
                                                const QGpgME::Protocol *cryptoProto)
{
    QString text = QStringLiteral("<ul>");
    for (const auto &recipientIt : recipients) {
        const auto recipient = recipientIt.first;
        const auto key = recipientIt.second;
        if (key.keyID()) {
            const auto link = QStringLiteral("messageviewer:showCertificate#%1 ### %2 ### %3")
                                  .arg(cryptoProto->displayName(), cryptoProto->name(), QString::fromLatin1(key.keyID()));
            text +=
                QStringLiteral("<li>%1 (<a href=\"%2\")Ox%3</a>)</li>").arg(QString::fromLatin1(key.userID(0).id()), link, QString::fromLatin1(key.keyID()));
        } else {
            const auto link = QStringLiteral("messageviewer:showCertificate#%1 ### %2 ### %3")
                                  .arg(cryptoProto->displayName(), cryptoProto->name(), QString::fromLatin1(recipient.keyID()));
            text += QStringLiteral("<li>%1 (<a href=\"%2\">0x%3</a>)</li>").arg(i18nc("@info", "Unknow Key"), link, QString::fromLatin1(recipient.keyID()));
        }
    }
    text += QStringLiteral("</ul>");
    return text;
}

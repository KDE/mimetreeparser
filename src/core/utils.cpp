// SPDX-FileCopyrightText: 2016 Sandro Knauß <sknauss@kde.org>
// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "utils.h"

#include <KLocalizedString>
#include <Libkleo/Formatting>

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
            QString displayName = QString::fromLatin1(key.userID(0).id());
            if (cryptoProto == QGpgME::smime()) {
                Kleo::DN dn(displayName);
                displayName = dnToDisplayName(dn);
            }
            displayName = displayName.toHtmlEscaped();
            const auto link = QStringLiteral("messageviewer:showCertificate#%1 ### %2 ### %3")
                                  .arg(cryptoProto->displayName(), cryptoProto->name(), QString::fromLatin1(key.keyID()));
            text += QStringLiteral("<li>%1 (<a href=\"%2\">%3</a>)</li>").arg(displayName, link, Kleo::Formatting::prettyID(key.keyID()));
        } else {
            const auto link = QStringLiteral("messageviewer:showCertificate#%1 ### %2 ### %3")
                                  .arg(cryptoProto->displayName(), cryptoProto->name(), QString::fromLatin1(recipient.keyID()));
            text +=
                QStringLiteral("<li>%1 (<a href=\"%2\">%3</a>)</li>").arg(i18nc("@info", "Unknown Key"), link, Kleo::Formatting::prettyID(recipient.keyID()));
        }
    }
    text += QStringLiteral("</ul>");
    return text;
}

QString MimeTreeParser::dnToDisplayName(const Kleo::DN &dn)
{
    QString displayName = dn[QStringLiteral("CN")];
    if (displayName.isEmpty()) {
        // In case there is no CN, put the full DN as display name
        displayName = dn.prettyDN();
    } else if (!dn[QStringLiteral("O")].isEmpty()) {
        displayName += i18nc("Separator", " - ") + dn[QStringLiteral("O")];
    }
    return displayName;
}

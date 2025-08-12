// SPDX-FileCopyrightText: 2016 Sandro Knauß <sknauss@kde.org>
// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "utils.h"
using namespace Qt::Literals::StringLiterals;

#include <KLocalizedString>

#include <Libkleo/DnAttributes>
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
    QString text = u"<ul>"_s;
    for (const auto &recipientIt : recipients) {
        const auto recipient = recipientIt.first;
        const auto key = recipientIt.second;
        if (key.keyID()) {
            QString displayName = QString::fromLatin1(key.userID(0).id());
            if (cryptoProto == QGpgME::smime()) {
                QGpgME::DN dn(displayName);
                dn.setAttributeOrder(Kleo::DNAttributes::order());
                displayName = dnToDisplayName(dn);
            }
            displayName = displayName.toHtmlEscaped();
            const auto link =
                u"messageviewer:showCertificate#%1 ### %2 ### %3"_s.arg(cryptoProto->displayName(), cryptoProto->name(), QString::fromLatin1(key.keyID()));
            text += u"<li>%1 (<a href=\"%2\">%3</a>)</li>"_s.arg(displayName, link, Kleo::Formatting::prettyID(key.keyID()));
        } else {
            const auto link = u"messageviewer:showCertificate#%1 ### %2 ### %3"_s.arg(cryptoProto->displayName(),
                                                                                      cryptoProto->name(),
                                                                                      QString::fromLatin1(recipient.keyID()));
            text += u"<li>%1 (<a href=\"%2\">%3</a>)</li>"_s.arg(i18nc("@info", "Unknown Key"), link, Kleo::Formatting::prettyID(recipient.keyID()));
        }
    }
    text += u"</ul>"_s;
    return text;
}

QString MimeTreeParser::dnToDisplayName(const QGpgME::DN &dn)
{
    QString displayName = dn[u"CN"_s];
    if (displayName.isEmpty()) {
        // In case there is no CN, put the full DN as display name
        displayName = dn.prettyDN();
    } else if (!dn[u"O"_s].isEmpty()) {
        displayName += i18nc("Separator", " - ") + dn[u"O"_s];
    }
    return displayName;
}

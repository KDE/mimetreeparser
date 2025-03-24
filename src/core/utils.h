// SPDX-FileCopyrightText: 2016 Sandro Knauß <sknauss@kde.org>
// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include <KMime/Content>

#include <QGpgME/DN>
#include <QGpgME/Protocol>

#include <gpgme++/decryptionresult.h>
#include <gpgme++/key.h>

#include <vector>

#include "mimetreeparser_core_export.h"

namespace MimeTreeParser
{
[[nodiscard]] KMime::Content *findTypeInDirectChildren(KMime::Content *content, const QByteArray &mimeType);

/// Convert a list of recipients to an html list
[[nodiscard]] MIMETREEPARSER_CORE_EXPORT QString
decryptRecipientsToHtml(const std::vector<std::pair<GpgME::DecryptionResult::Recipient, GpgME::Key>> &recipients, const QGpgME::Protocol *cryptoProto);

/// Convert a DN to a more compact display name (usually just Common name + organization name)
[[nodiscard]] MIMETREEPARSER_CORE_EXPORT QString dnToDisplayName(const QGpgME::DN &dn);
}

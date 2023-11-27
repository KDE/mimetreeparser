// SPDX-FileCopyrightText: 2016 Sandro Knauß <sknauss@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include <KMime/Content>

#include <QGpgME/Protocol>
#include <gpgme++/decryptionresult.h>
#include <gpgme++/key.h>

#include <vector>

#include "mimetreeparser_core_export.h"

namespace MimeTreeParser
{
KMime::Content *findTypeInDirectChildren(KMime::Content *content, const QByteArray &mimeType);

/// Convert a list of recipients to an html list
MIMETREEPARSER_CORE_EXPORT QString decryptRecipientsToHtml(const std::vector<std::pair<GpgME::DecryptionResult::Recipient, GpgME::Key>> &recipients,
                                                           const QGpgME::Protocol *cryptoProto);
}

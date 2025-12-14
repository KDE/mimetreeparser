// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include "mimetreeparser_core_export.h"

#include <KMime/Message>

namespace MimeTreeParser
{
namespace Core
{
namespace FileOpener
{
/// Open messages from file
[[nodiscard]] MIMETREEPARSER_CORE_EXPORT QList<std::shared_ptr<KMime::Message>> openFile(const QString &fileName);
}
}
}

// SPDX-FileCopyrigthText: 2023 Carl Schwan <carl@carlschwan.eu>
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
QVector<KMime::Message::Ptr> MIMETREEPARSER_CORE_EXPORT openFile(const QString &fileName);
}
}
}

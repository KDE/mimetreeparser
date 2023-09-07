// SPDX-FileCopyrightText: 2017 Daniel Vrátil <dvratil@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <KMime/Message>

namespace MimeTreeParser
{
namespace Widgets
{
namespace CryptoUtils
{
Q_REQUIRED_RESULT KMime::Message::Ptr decryptMessage(const KMime::Message::Ptr &decrypt, bool &wasEncrypted);
}
}
}

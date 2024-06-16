// SPDX-FileCopyrightText: 2004 Marc Mutz <mutz@kde.org>
// SPDX-FileCopyrightText: 2004 Ingo Kloecker <kloecker@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "messagepart.h"

namespace KMime
{
class Content;
}

namespace MimeTreeParser
{
class ObjectTreeParser;

namespace Interface
{

class BodyPart;

///
class MIMETREEPARSER_CORE_EXPORT BodyPartFormatter
{
public:
    virtual ~BodyPartFormatter()
    {
    }

    virtual MessagePart::Ptr process(ObjectTreeParser *otp, KMime::Content *node) const;
    virtual QList<MessagePart::Ptr> processList(ObjectTreeParser *otp, KMime::Content *node) const;
};

/// \short interface for BodyPartFormatter plugins
///
/// The interface is queried by for types, subtypes, and the
/// corresponding bodypart formatter, and the result inserted into
/// the bodypart formatter factory.
///
/// Subtype alone or both type and subtype may be "*", which is
/// taken as a wildcard, so that e.g. type=text subtype=* matches
/// any text subtype, but with lesser specificity than a concrete
/// mimetype such as text/plain. type=* is only allowed when
/// subtype=*, too.
class MIMETREEPARSER_CORE_EXPORT BodyPartFormatterPlugin
{
public:
    virtual ~BodyPartFormatterPlugin();

    virtual const BodyPartFormatter *bodyPartFormatter(const QString &mimetype) const = 0;
};

} // namespace Interface
} // namespace MimeTreeParser

Q_DECLARE_INTERFACE(MimeTreeParser::Interface::BodyPartFormatterPlugin, "org.kde.messageviewer.bodypartformatter/2.0")

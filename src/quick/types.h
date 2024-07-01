// SPDX-FileCopyrightText: 2024 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include <MimeTreeParserCore/AttachmentModel>
#include <MimeTreeParserCore/MessageParser>
#include <MimeTreeParserCore/PartModel>

#include <QtQml/qqmlregistration.h>

class MessageParserForeign : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(MessageParser)
    QML_FOREIGN(MessageParser)
};

class PartModelForeign : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(PartModel)
    QML_UNCREATABLE("Enum only")
    QML_FOREIGN(PartModel)
};

class AttachmentModelForeign : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(AttachmentModel)
    QML_UNCREATABLE("Enum only")
    QML_FOREIGN(AttachmentModel)
};

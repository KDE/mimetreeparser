// SPDX-FileCopyrightText: 2022 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "mimetreeparserplugin.h"

#include <MimeTreeParserCore/AttachmentModel>
#include <MimeTreeParserCore/MessageParser>
#include <MimeTreeParserCore/PartModel>
#include <QQmlEngine>

void MimeTreeParserPlugin::registerTypes(const char *uri)
{
    Q_ASSERT(uri == QByteArray("org.kde.pim.mimetreeparser"));

    qmlRegisterModule(uri, 1, 0);
    qmlRegisterType<MessageParser>(uri, 1, 0, "MessageParser");
    qRegisterMetaType<PartModel::Types>("PartModel::Types");
    qmlRegisterUncreatableType<PartModel>(uri, 1, 0, "PartModel", QStringLiteral("not instanciated"));
    qmlRegisterUncreatableType<AttachmentModel>(uri, 1, 0, "AttachmentModel", QStringLiteral("not instanciated"));
}

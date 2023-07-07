// SPDX-FileCopyrightText: 2022 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "mimetreeparserplugin.h"

#include <MimeTreeParser/MessageParser>
#include <QQmlEngine>

void MimeTreeParserPlugin::registerTypes(const char *uri)
{
    Q_ASSERT(uri == QByteArray("org.kde.pim.mimetreeparser"));

    qmlRegisterModule(uri, 1, 0);
    qmlRegisterType<MessageParser>(uri, 1, 0, "MessageParser");
}

void MimeTreeParserPlugin::initializeEngine(QQmlEngine *engine, const char *uri)
{
    Q_UNUSED(uri);
}

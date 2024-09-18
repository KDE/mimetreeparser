// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include "mimetreeparser_core_export.h"

#include <QObject>
#include <QWindow>

class MIMETREEPARSER_CORE_EXPORT UrlHandler : public QObject
{
    Q_OBJECT

public:
    explicit UrlHandler(QObject *parent = nullptr);

    Q_INVOKABLE bool handleClick(const QUrl &url, QWindow *window);

Q_SIGNALS:
    void errorOccurred(const QString &errorMessage);
};

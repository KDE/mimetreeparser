// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include "mimetreeparser_core_export.h"

#include <QObject>
#include <QWindow>
/*!
 * \class MimeTreeParser::UrlHandler
 * \inmodule MimeTreeParserCore
 * \inheaderfile MimeTreeParserCore/UrlHandler
 */
class MIMETREEPARSER_CORE_EXPORT UrlHandler : public QObject
{
    Q_OBJECT

public:
    /*!
     * \brief Constructs a UrlHandler
     * \param parent The parent object
     */
    explicit UrlHandler(QObject *parent = nullptr);

    /*!
     * \brief Handles a click on a URL
     * \param url The URL that was clicked
     * \param window The window context for handling the URL
     * \return True if the URL was handled successfully, false otherwise
     */
    Q_INVOKABLE bool handleClick(const QUrl &url, QWindow *window);

Q_SIGNALS:
    /*!
     * \brief Emitted when an error occurs while handling a URL
     * \param errorMessage The description of the error
     */
    void errorOccurred(const QString &errorMessage);

private:
    [[nodiscard]] bool foundSMIMEData(const QString &aUrl, QString &displayName, QString &libName, QString &keyId);
};

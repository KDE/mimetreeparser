// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include "mimetreeparser_widgets_export.h"

#include <MimeTreeParserCore/PartModel>
#include <QFrame>

class SignatureInfo;
class QPaintEvent;
class UrlHandler;

/// \internal
class MIMETREEPARSER_WIDGETS_EXPORT MessageWidgetContainer : public QFrame
{
    Q_OBJECT

public:
    /*!
     * \brief Constructs a MessageWidgetContainer
     * \param signatureInfo The signature information text
     * \param signatureIconName The icon name for signature
     * \param signatureSecurityLevel The security level of the signature
     * \param encryptionInfo The encryption information
     * \param encryptionIconName The icon name for encryption
     * \param encryptionSecurityLevel The security level of the encryption
     * \param sidebarSecurityLevel The overall sidebar security level
     * \param urlHandler The URL handler for handling links
     * \param parent The parent widget
     */
    explicit MessageWidgetContainer(const QString &signatureInfo,
                                    const QString &signatureIconName,
                                    PartModel::SecurityLevel signatureSecurityLevel,
                                    const SignatureInfo &encryptionInfo,
                                    const QString &encryptionIconName,
                                    PartModel::SecurityLevel encryptionSecurityLevel,
                                    PartModel::SecurityLevel sidebarSecurityLevel,
                                    UrlHandler *urlHandler,
                                    QWidget *parent = nullptr);
    /*!
     * \brief Destroys the MessageWidgetContainer
     */
    ~MessageWidgetContainer() override;

protected:
    /*!
     * \brief Handles paint events to draw the container
     * \param event The paint event
     */
    void paintEvent(QPaintEvent *event) override;
    /*!
     * \brief Handles generic events for the container
     * \param event The event to handle
     * \return True if the event was handled
     */
    [[nodiscard]] bool event(QEvent *event) override;

private:
    MIMETREEPARSER_WIDGETS_NO_EXPORT void createLayout();

    QString const m_signatureInfo;
    PartModel::SecurityLevel m_signatureSecurityLevel;
    bool m_displaySignatureInfo;
    QString const m_signatureIconName;

    SignatureInfo const m_encryptionInfo;
    PartModel::SecurityLevel m_encryptionSecurityLevel;
    bool m_displayEncryptionInfo;
    QString const m_encryptionIconName;

    PartModel::SecurityLevel m_sidebarSecurityLevel;

    UrlHandler *const m_urlHandler;
};

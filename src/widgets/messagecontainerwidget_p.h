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
    explicit MessageWidgetContainer(bool isSigned,
                                    const SignatureInfo &signatureInfo,
                                    PartModel::SecurityLevel signatureSecurityLevel,
                                    bool displaySignatureInfo,
                                    bool isEncrypted,
                                    const SignatureInfo &encryptionInfo,
                                    PartModel::SecurityLevel encryptionSecurityLevel,
                                    bool displayEncryptionInfo,
                                    UrlHandler *urlHandler,
                                    QWidget *parent = nullptr);
    ~MessageWidgetContainer() override;

protected:
    void paintEvent(QPaintEvent *event) override;
    bool event(QEvent *event) override;

private:
    MIMETREEPARSER_WIDGETS_NO_EXPORT void createLayout();

    bool m_isSigned;
    SignatureInfo const m_signatureInfo;
    PartModel::SecurityLevel m_signatureSecurityLevel;
    bool m_displaySignatureInfo;

    bool m_isEncrypted;
    SignatureInfo const m_encryptionInfo;
    PartModel::SecurityLevel m_encryptionSecurityLevel;
    bool m_displayEncryptionInfo;

    UrlHandler *const m_urlHandler;
};

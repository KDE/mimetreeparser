// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include <MimeTreeParserCore/PartModel>
#include <QFrame>

class SignatureInfo;
class QPaintEvent;
class QResizeEvent;
class UrlHandler;

class MessageWidgetContainer : public QFrame
{
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
    ~MessageWidgetContainer();

protected:
    void paintEvent(QPaintEvent *event) override;
    bool event(QEvent *event) override;

private:
    void createLayout();

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

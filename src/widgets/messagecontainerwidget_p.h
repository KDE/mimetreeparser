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
    explicit MessageWidgetContainer(const QString &signatureInfo,
                                    const QString &signatureIconName,
                                    PartModel::SecurityLevel signatureSecurityLevel,
                                    const SignatureInfo &encryptionInfo,
                                    const QString &encryptionIconName,
                                    PartModel::SecurityLevel encryptionSecurityLevel,
                                    PartModel::SecurityLevel sidebarSecurityLevel,
                                    UrlHandler *urlHandler,
                                    QWidget *parent = nullptr);
    ~MessageWidgetContainer() override;

protected:
    void paintEvent(QPaintEvent *event) override;
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

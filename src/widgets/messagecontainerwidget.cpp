// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "messagecontainerwidget_p.h"

#include <KLocalizedString>
#include <KMessageWidget>
#include <Libkleo/Compliance>

#include <QAction>
#include <QIcon>
#include <QPaintEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QVBoxLayout>

namespace
{

const int borderWidth = 5;

QColor getColor(PartModel::SecurityLevel securityLevel)
{
    if (securityLevel == PartModel::Good) {
        return QColor(39, 174, 96); // Window: ForegroundPositive
    }
    if (securityLevel == PartModel::Bad) {
        return QColor(218, 68, 83); // Window: ForegroundNegative
    }
    if (securityLevel == PartModel::NotSoGood) {
        return QColor(246, 116, 0); // Window: ForegroundNeutral
    }
    return QColor();
}

KMessageWidget::MessageType getType(PartModel::SecurityLevel securityLevel)
{
    if (securityLevel == PartModel::Good) {
        return KMessageWidget::MessageType::Positive;
    }
    if (securityLevel == PartModel::Bad) {
        return KMessageWidget::MessageType::Error;
    }
    if (securityLevel == PartModel::NotSoGood) {
        return KMessageWidget::MessageType::Warning;
    }
    return KMessageWidget::MessageType::Information;
}

QString getDetails(SignatureInfo *signatureDetails)
{
    QString details;
    if (signatureDetails->keyMissing) {
        if (Kleo::DeVSCompliance::isCompliant() && signatureDetails->isCompliant) {
            details += i18ndc("mimetreeparser",
                              "@label",
                              "This message has been signed VS-NfD compliant using the key %1.",
                              QString::fromUtf8(signatureDetails->keyId))
                + QLatin1Char('\n');
        } else {
            details += i18ndc("mimetreeparser", "@label", "This message has been signed using the key %1.", QString::fromUtf8(signatureDetails->keyId))
                + QLatin1Char('\n');
        }
        details += i18ndc("mimetreeparser", "@label", "The key details are not available.");
    } else {
        if (Kleo::DeVSCompliance::isCompliant() && signatureDetails->isCompliant) {
            details += i18ndc("mimetreeparser", "@label", "This message has been signed VS-NfD compliant by %1.", signatureDetails->signer);
        } else {
            details += i18ndc("mimetreeparser", "@label", "This message has been signed by %1.", signatureDetails->signer);
        }
        if (signatureDetails->keyRevoked) {
            details += QLatin1Char('\n') + i18ndc("mimetreeparser", "@label", "The key was revoked.");
        }
        if (signatureDetails->keyExpired) {
            details += QLatin1Char('\n') + i18ndc("mimetreeparser", "@label", "The key has expired.");
        }
        if (signatureDetails->keyIsTrusted) {
            details += QLatin1Char('\n') + i18ndc("mimetreeparser", "@label", "You are trusting this key.");
        }
        if (!signatureDetails->signatureIsGood && !signatureDetails->keyRevoked && !signatureDetails->keyExpired && !signatureDetails->keyIsTrusted) {
            details += QLatin1Char('\n') + i18ndc("mimetreeparser", "@label", "The signature is invalid.");
        }
    }
    return details;
}

}

MessageWidgetContainer::MessageWidgetContainer(bool isSigned,
                                               SignatureInfo *signatureInfo,
                                               PartModel::SecurityLevel signatureSecurityLevel,
                                               bool isEncrypted,
                                               SignatureInfo *encryptionInfo,
                                               PartModel::SecurityLevel encryptionSecurityLevel,
                                               QWidget *parent)
    : QFrame(parent)
    , m_isSigned(isSigned)
    , m_signatureInfo(signatureInfo)
    , m_signatureSecurityLevel(signatureSecurityLevel)
    , m_isEncrypted(isEncrypted)
    , m_encryptionInfo(encryptionInfo)
    , m_encryptionSecurityLevel(encryptionSecurityLevel)
{
    m_signatureInfo->setParent(this);
    m_encryptionInfo->setParent(this);
    createLayout();
}

MessageWidgetContainer::~MessageWidgetContainer() = default;

void MessageWidgetContainer::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);

    if (layoutDirection() == Qt::RightToLeft) {
        // todo
    } else {
        auto r = rect();
        r.setWidth(borderWidth);
        const QColor color = getColor(PartModel::SecurityLevel::Good);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(QColor(color));
        painter.setPen(QPen(Qt::NoPen));
        painter.drawRect(r);
    }
}

bool MessageWidgetContainer::event(QEvent *event)
{
    if (event->type() == QEvent::Polish && !layout()) {
        createLayout();
    }

    return QFrame::event(event);
}

void MessageWidgetContainer::createLayout()
{
    delete layout();

    auto vLayout = new QVBoxLayout(this);

    setLayout(vLayout);
    layout()->setContentsMargins(borderWidth * 2, 0, 0, 0);

    if (m_isEncrypted) {
        auto encryptionMessage = new KMessageWidget(this);
        encryptionMessage->setMessageType(getType(m_encryptionSecurityLevel));
        encryptionMessage->setCloseButtonVisible(false);
        encryptionMessage->setIcon(QIcon::fromTheme(QStringLiteral("mail-encrypted")));
        encryptionMessage->setWordWrap(true);

        if (m_encryptionInfo->keyId.isEmpty()) {
            if (Kleo::DeVSCompliance::isCompliant() && m_encryptionInfo->isCompliant) {
                encryptionMessage->setText(
                    i18n("This message is VS-NfD compliant encrypted but we don't have the key for it.", QString::fromUtf8(m_encryptionInfo->keyId)));
            } else {
                encryptionMessage->setText(i18n("This message is encrypted but we don't have the key for it."));
            }
        } else {
            if (Kleo::DeVSCompliance::isCompliant() && m_encryptionInfo->isCompliant) {
                encryptionMessage->setText(i18n("This message is VS-NfD compliant encrypted."));
            } else {
                encryptionMessage->setText(i18n("This message is encrypted."));
            }
        }
        vLayout->addWidget(encryptionMessage);
    }

    if (m_isSigned) {
        auto signatureMessage = new KMessageWidget(this);
        signatureMessage->setIcon(QIcon::fromTheme(QStringLiteral("mail-signed")));
        signatureMessage->setCloseButtonVisible(false);
        signatureMessage->setText(getDetails(m_signatureInfo));
        signatureMessage->setMessageType(getType(m_signatureSecurityLevel));
        signatureMessage->setWordWrap(true);
        vLayout->addWidget(signatureMessage);
    }
}

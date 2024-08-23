// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "../core/utils.h"
#include "messagecontainerwidget_p.h"
#include "partmodel.h"
#include "urlhandler_p.h"

#include <KLocalizedString>
#include <KMessageWidget>
#include <Libkleo/Compliance>
#include <Libkleo/Formatting>
#include <QGpgME/Protocol>

#include <QLabel>
#include <QPaintEvent>
#include <QPainter>
#include <QVBoxLayout>
#include <verificationresult.h>

namespace
{

const int borderWidth = 5;

QColor getColor(PartModel::SecurityLevel securityLevel)
{
    const static QHash<PartModel::SecurityLevel, QColor> colors{
        {PartModel::Good, QColor(39, 174, 96)}, // Window: ForegroundPositive
        {PartModel::Bad, QColor(218, 68, 83)}, // Window: ForegroundNegative
        {PartModel::NotSoGood, QColor(246, 116, 0)}, // Window: ForegroundNeutral
    };

    return colors.value(securityLevel, QColor());
}

KMessageWidget::MessageType getType(PartModel::SecurityLevel securityLevel)
{
    const static QHash<PartModel::SecurityLevel, KMessageWidget::MessageType> messageTypes{
        {PartModel::Good, KMessageWidget::MessageType::Positive},
        {PartModel::Bad, KMessageWidget::MessageType::Error},
        {PartModel::NotSoGood, KMessageWidget::MessageType::Warning},
    };

    return messageTypes.value(securityLevel, KMessageWidget::MessageType::Information);
}
}

MessageWidgetContainer::MessageWidgetContainer(const QString &signatureInfo,
                                               const QString &signatureIconName,
                                               PartModel::SecurityLevel signatureSecurityLevel,
                                               const SignatureInfo &encryptionInfo,
                                               const QString &encryptionIconName,
                                               PartModel::SecurityLevel encryptionSecurityLevel,
                                               PartModel::SecurityLevel sidebarSecurityLevel,
                                               UrlHandler *urlHandler,
                                               QWidget *parent)
    : QFrame(parent)
    , m_signatureInfo(signatureInfo)
    , m_signatureSecurityLevel(signatureSecurityLevel)
    , m_displaySignatureInfo(signatureSecurityLevel != PartModel::Unknow)
    , m_signatureIconName(signatureIconName)
    , m_encryptionInfo(encryptionInfo)
    , m_encryptionSecurityLevel(encryptionSecurityLevel)
    , m_displayEncryptionInfo(encryptionSecurityLevel != PartModel::Unknow)
    , m_encryptionIconName(encryptionIconName)
    , m_sidebarSecurityLevel(sidebarSecurityLevel)
    , m_urlHandler(urlHandler)
{
    createLayout();
}

MessageWidgetContainer::~MessageWidgetContainer() = default;

void MessageWidgetContainer::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    if (m_sidebarSecurityLevel == PartModel::Unknow) {
        return;
    }

    QPainter painter(this);
    if (layoutDirection() == Qt::RightToLeft) {
        auto r = rect();
        r.setX(width() - borderWidth);
        r.setWidth(borderWidth);
        const QColor color = getColor(m_sidebarSecurityLevel);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(QColor(color));
        painter.setPen(QPen(Qt::NoPen));
        painter.drawRect(r);
    } else {
        auto r = rect();
        r.setWidth(borderWidth);
        const QColor color = getColor(m_sidebarSecurityLevel);
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

    if (m_sidebarSecurityLevel == PartModel::Unknow) {
        return;
    }

    if (layoutDirection() == Qt::RightToLeft) {
        layout()->setContentsMargins(0, 0, borderWidth * 2, 0);
    } else {
        layout()->setContentsMargins(borderWidth * 2, 0, 0, 0);
    }

    if (m_displayEncryptionInfo) {
        auto encryptionMessage = new KMessageWidget(this);
        encryptionMessage->setObjectName(QLatin1StringView("EncryptionMessage"));
        encryptionMessage->setCloseButtonVisible(false);
        encryptionMessage->setMessageType(getType(m_encryptionSecurityLevel));

        QString text;
        if (m_encryptionSecurityLevel == PartModel::Bad) {
            encryptionMessage->setIcon(QIcon::fromTheme(QStringLiteral("data-error")));
            if (Kleo::DeVSCompliance::isCompliant() && m_encryptionInfo.isCompliant) {
                text = i18n("This message is VS-NfD compliant encrypted but you don't have a matching secret key.", QString::fromUtf8(m_encryptionInfo.keyId));
            } else {
                text = i18n("This message is encrypted but you don't have a matching secret key.");
            }
        } else {
            encryptionMessage->setIcon(QIcon::fromTheme(QStringLiteral("mail-encrypted")));
            if (Kleo::DeVSCompliance::isCompliant() && m_encryptionInfo.isCompliant) {
                text = i18n("This message is VS-NfD compliant encrypted.");
            } else {
                text = i18n("This message is encrypted.");
            }
        }

        encryptionMessage->setText(text + QLatin1Char(' ') + QStringLiteral("<a href=\"messageviewer:showDetails\">Details</a>"));

        connect(encryptionMessage, &KMessageWidget::linkActivated, this, [this, encryptionMessage, text](const QString &link) {
            QUrl url(link);
            if (url.path() == QStringLiteral("showDetails")) {
                QString newText = text + QLatin1Char(' ') + i18n("The message is encrypted for the following recipients:");

                newText += MimeTreeParser::decryptRecipientsToHtml(m_encryptionInfo.decryptRecipients, m_encryptionInfo.cryptoProto);

                encryptionMessage->setText(newText);
                return;
            }

            if (url.path() == QStringLiteral("showCertificate")) {
                m_urlHandler->handleClick(QUrl(link), window()->windowHandle());
            }
        });

        vLayout->addWidget(encryptionMessage);
    }

    if (m_displaySignatureInfo) {
        auto signatureMessage = new KMessageWidget(this);
        signatureMessage->setObjectName(QStringLiteral("SignatureMessage"));
        signatureMessage->setCloseButtonVisible(false);
        signatureMessage->setText(m_signatureInfo);
        connect(signatureMessage, &KMessageWidget::linkActivated, this, [this](const QString &link) {
            m_urlHandler->handleClick(QUrl(link), window()->windowHandle());
        });
        signatureMessage->setMessageType(getType(m_signatureSecurityLevel));
        signatureMessage->setWordWrap(true);
        signatureMessage->setIcon(QIcon::fromTheme(m_signatureIconName));

        vLayout->addWidget(signatureMessage);
    }
}

#include "moc_messagecontainerwidget_p.cpp"

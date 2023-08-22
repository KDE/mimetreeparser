// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "messagecontainerwidget_p.h"
#include "urlhandler_p.h"

#include <KLocalizedString>
#include <KMessageWidget>
#include <Libkleo/Compliance>
#include <QGpgME/Protocol>

#include <QDebug>
#include <QLabel>
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

QString getDetails(const SignatureInfo &signatureDetails)
{
    QString href;
    if (signatureDetails.cryptoProto) {
        href = QStringLiteral("messageviewer:showCertificate#%1 ### %2 ### %3")
                   .arg(signatureDetails.cryptoProto->displayName(), signatureDetails.cryptoProto->name(), QString::fromLatin1(signatureDetails.keyId));
    }

    QString details;
    if (signatureDetails.keyMissing) {
        if (Kleo::DeVSCompliance::isCompliant() && signatureDetails.isCompliant) {
            details += i18ndc("mimetreeparser",
                              "@label",
                              "This message has been signed VS-NfD compliant using the key <a href=\"%1\">%2</a>.",
                              href,
                              QString::fromUtf8(signatureDetails.keyId))
                + QLatin1Char('\n');
        } else {
            details += i18ndc("mimetreeparser",
                              "@label",
                              "This message has been signed using the key <a href=\"%1\">%2</a>.",
                              href,
                              QString::fromUtf8(signatureDetails.keyId))
                + QLatin1Char('\n');
        }
        details += i18ndc("mimetreeparser", "@label", "The key details are not available.");
    } else {
        if (Kleo::DeVSCompliance::isCompliant() && signatureDetails.isCompliant) {
            details += i18ndc("mimetreeparser", "@label", "This message has been signed VS-NfD compliant by %1.", signatureDetails.signer.toHtmlEscaped());
        } else {
            details += i18ndc("mimetreeparser", "@label", "This message has been signed by %1.", signatureDetails.signer.toHtmlEscaped());
        }
        if (signatureDetails.keyRevoked) {
            details += QLatin1Char('\n') + i18ndc("mimetreeparser", "@label", "The <a href=\"%1\">key</a> was revoked.", href);
        }
        if (signatureDetails.keyExpired) {
            details += QLatin1Char('\n') + i18ndc("mimetreeparser", "@label", "The <a href=\"%1\">key</a> was expired.", href);
        }

        if (signatureDetails.keyTrust == GpgME::Signature::Unknown) {
            details +=
                QLatin1Char(' ') + i18ndc("mimetreeparser", "@label", "The signature is valid, but the <a href=\"%1\">key</a>'s validity is unknown.", href);
        } else if (signatureDetails.keyTrust == GpgME::Signature::Marginal) {
            details +=
                QLatin1Char(' ') + i18ndc("mimetreeparser", "@label", "The signature is valid and the <a href=\"%1\">key</a> is marginally trusted.", href);
        } else if (signatureDetails.keyTrust == GpgME::Signature::Full) {
            details += QLatin1Char(' ') + i18ndc("mimetreeparser", "@label", "The signature is valid and the <a href=\"%1\">key</a> is fully trusted.", href);
        } else if (signatureDetails.keyTrust == GpgME::Signature::Ultimate) {
            details +=
                QLatin1Char(' ') + i18ndc("mimetreeparser", "@label", "The signature is valid and the <a href=\"%1\">key</a> is ultimately trusted.", href);
        } else {
            details += QLatin1Char(' ') + i18ndc("mimetreeparser", "@label", "The signature is valid, but the <a href=\"%1\">key</a> is untrusted.", href);
        }
        if (!signatureDetails.signatureIsGood && !signatureDetails.keyRevoked && !signatureDetails.keyExpired
            && signatureDetails.keyTrust != GpgME::Signature::Unknown) {
            details += QLatin1Char(' ') + i18ndc("mimetreeparser", "@label", "The signature is invalid.");
        }
    }
    return details;
}

}

MessageWidgetContainer::MessageWidgetContainer(bool isSigned,
                                               const SignatureInfo &signatureInfo,
                                               PartModel::SecurityLevel signatureSecurityLevel,
                                               bool isEncrypted,
                                               const SignatureInfo &encryptionInfo,
                                               PartModel::SecurityLevel encryptionSecurityLevel,
                                               UrlHandler *urlHandler,
                                               QWidget *parent)
    : QFrame(parent)
    , m_isSigned(isSigned)
    , m_signatureInfo(signatureInfo)
    , m_signatureSecurityLevel(signatureSecurityLevel)
    , m_isEncrypted(isEncrypted)
    , m_encryptionInfo(encryptionInfo)
    , m_encryptionSecurityLevel(encryptionSecurityLevel)
    , m_urlHandler(urlHandler)
{
    createLayout();
}

MessageWidgetContainer::~MessageWidgetContainer() = default;

void MessageWidgetContainer::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    if (!m_isSigned && !m_isEncrypted) {
        return;
    }

    QPainter painter(this);
    if (layoutDirection() == Qt::RightToLeft) {
        auto r = rect();
        r.setX(width() - borderWidth);
        r.setWidth(borderWidth);
        const QColor color = getColor(PartModel::SecurityLevel::Good);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(QColor(color));
        painter.setPen(QPen(Qt::NoPen));
        painter.drawRect(r);
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
    if (m_isSigned || m_isEncrypted) {
        if (layoutDirection() == Qt::RightToLeft) {
            layout()->setContentsMargins(0, 0, borderWidth * 2, 0);
        } else {
            layout()->setContentsMargins(borderWidth * 2, 0, 0, 0);
        }
    }

    if (m_isEncrypted) {
        auto encryptionMessage = new KMessageWidget(this);
        encryptionMessage->setMessageType(getType(m_encryptionSecurityLevel));
        encryptionMessage->setCloseButtonVisible(false);
        encryptionMessage->setIcon(QIcon::fromTheme(QStringLiteral("mail-encrypted")));

        QString text;
        if (m_encryptionInfo.keyId.isEmpty()) {
            if (Kleo::DeVSCompliance::isCompliant() && m_encryptionInfo.isCompliant) {
                text = i18n("This message is VS-NfD compliant encrypted but we don't have the key for it.", QString::fromUtf8(m_encryptionInfo.keyId));
            } else {
                text = i18n("This message is encrypted but we don't have the key for it.");
            }
        } else {
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
                QString newText = text + QStringLiteral(" ") + i18n("The message is encrypted for the following keys:") + QStringLiteral("<ul>");

                for (const auto &recipient : m_encryptionInfo.decryptRecipients) {
                    if (recipient.second.keyID()) {
                        const auto href = QStringLiteral("messageviewer:showCertificate#%1 ### %2 ### %3")
                                              .arg(m_encryptionInfo.cryptoProto->displayName(),
                                                   m_encryptionInfo.cryptoProto->name(),
                                                   QString::fromLatin1(recipient.second.keyID()));

                        newText += QStringLiteral("<li><a href=\"%1\">0x%2</a></li>").arg(href, QString::fromLatin1(recipient.second.keyID()));
                    } else {
                        const auto href = QStringLiteral("messageviewer:showCertificate#%1 ### %2 ### %3")
                                              .arg(m_encryptionInfo.cryptoProto->displayName(),
                                                   m_encryptionInfo.cryptoProto->name(),
                                                   QString::fromLatin1(recipient.first.keyID()));

                        newText +=
                            QStringLiteral("<li><a href=\"%1\">0x%2</a> (%3)</li>").arg(href, QString::fromLatin1(recipient.first.keyID()), i18n("Unknow key"));
                    }
                }

                newText += QStringLiteral("</ul>");

                encryptionMessage->setText(newText);
                return;
            }

            if (url.path() == QStringLiteral("showCertificate")) {
                m_urlHandler->handleClick(QUrl(link), window()->windowHandle());
            }
        });

        vLayout->addWidget(encryptionMessage);
    }

    if (m_isSigned) {
        auto signatureMessage = new KMessageWidget(this);
        signatureMessage->setIcon(QIcon::fromTheme(QStringLiteral("mail-signed")));
        signatureMessage->setCloseButtonVisible(false);
        signatureMessage->setText(getDetails(m_signatureInfo));
        connect(signatureMessage, &KMessageWidget::linkActivated, this, [this](const QString &link) {
            m_urlHandler->handleClick(QUrl(link), window()->windowHandle());
        });
        signatureMessage->setMessageType(getType(m_signatureSecurityLevel));
        signatureMessage->setWordWrap(true);

        vLayout->addWidget(signatureMessage);
    }
}

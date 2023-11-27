// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "../core/utils.h"
#include "messagecontainerwidget_p.h"
#include "urlhandler_p.h"

#include <KLocalizedString>
#include <KMessageWidget>
#include <Libkleo/Compliance>
#include <QGpgME/Protocol>

#include <QPaintEvent>
#include <QPainter>
#include <QVBoxLayout>

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
        QString signerDisplayName = signatureDetails.signer.toHtmlEscaped();
        if (signatureDetails.cryptoProto == QGpgME::smime()) {
            Kleo::DN dn(signatureDetails.signer);
            signerDisplayName = MimeTreeParser::dnToDisplayName(dn).toHtmlEscaped();
        }
        if (Kleo::DeVSCompliance::isCompliant() && signatureDetails.isCompliant) {
            details += i18ndc("mimetreeparser", "@label", "This message has been signed VS-NfD compliant by %1.", signerDisplayName);
        } else {
            details += i18ndc("mimetreeparser", "@label", "This message has been signed by %1.", signerDisplayName);
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
                                               bool displaySignatureInfo,
                                               bool isEncrypted,
                                               const SignatureInfo &encryptionInfo,
                                               PartModel::SecurityLevel encryptionSecurityLevel,
                                               bool displayEncryptionInfo,
                                               UrlHandler *urlHandler,
                                               QWidget *parent)
    : QFrame(parent)
    , m_isSigned(isSigned)
    , m_signatureInfo(signatureInfo)
    , m_signatureSecurityLevel(signatureSecurityLevel)
    , m_displaySignatureInfo(displaySignatureInfo)
    , m_isEncrypted(isEncrypted)
    , m_encryptionInfo(encryptionInfo)
    , m_encryptionSecurityLevel(encryptionSecurityLevel)
    , m_displayEncryptionInfo(displayEncryptionInfo)
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

    if (m_isSigned || m_isEncrypted) {
        if (layoutDirection() == Qt::RightToLeft) {
            layout()->setContentsMargins(0, 0, borderWidth * 2, 0);
        } else {
            layout()->setContentsMargins(borderWidth * 2, 0, 0, 0);
        }
    }

    if (m_isEncrypted && m_displayEncryptionInfo) {
        auto encryptionMessage = new KMessageWidget(this);
        encryptionMessage->setObjectName(QLatin1StringView("EncryptionMessage"));
        encryptionMessage->setCloseButtonVisible(false);
        encryptionMessage->setIcon(QIcon::fromTheme(QStringLiteral("mail-encrypted")));

        QString text;
        if (m_encryptionInfo.keyId.isEmpty()) {
            encryptionMessage->setMessageType(KMessageWidget::Error);
            if (Kleo::DeVSCompliance::isCompliant() && m_encryptionInfo.isCompliant) {
                text = i18n("This message is VS-NfD compliant encrypted but we don't have the key for it.", QString::fromUtf8(m_encryptionInfo.keyId));
            } else {
                text = i18n("This message is encrypted but we don't have the key for it.");
            }
        } else {
            encryptionMessage->setMessageType(KMessageWidget::Positive);
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
                QString newText = text + QLatin1Char(' ') + i18n("The message is encrypted for the following keys:");

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

    if (m_isSigned && m_displaySignatureInfo) {
        auto signatureMessage = new KMessageWidget(this);
        signatureMessage->setObjectName(QLatin1StringView("SignatureMessage"));
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

#include "moc_messagecontainerwidget_p.cpp"

// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "../core/utils.h"

#include "messagecontainerwidget_p.h"
#include "partmodel.h"
#include <MimeTreeParserCore/UrlHandler>

#include <KLocalizedString>
#include <KMessageWidget>
#include <KSqueezedTextLabel>
#include <Libkleo/Compliance>
#include <Libkleo/Formatting>
#include <QGpgME/Protocol>

#include <QLabel>
#include <QMimeDatabase>
#include <QPaintEvent>
#include <QPainter>
#include <QStyleOption>
#include <QVBoxLayout>

#include <gpgme++/verificationresult.h>

using namespace Qt::Literals::StringLiterals;

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

class AttachmentBox : public QFrame
{
public:
    AttachmentBox(const QList<QSharedPointer<MimeTreeParser::MessagePart>> &attachments, MessageWidgetContainer *parent)
        : QFrame(parent)
        , maxWidgetWidth(50)
        , oldWidth(width())
        , grid(nullptr)
    {
        setObjectName("AttachmentBox"); // for autotests
        setFrameStyle(QFrame::Box);

        QList<QWidget *> attachmentWidgets;
        for (const auto &attachment : attachments) {
            auto widget = new QWidget(this);
            auto innerLayout = new QHBoxLayout(widget);
            innerLayout->setContentsMargins({});

            const auto mimetype = QMimeDatabase().mimeTypeForName(QString::fromLatin1(attachment->mimeType()));
            auto icon = QIcon::fromTheme(mimetype.iconName());
            if (icon.isNull()) {
                icon = QIcon::fromTheme(u"unknown"_s);
            }
            auto pic = new QLabel();
            QStyleOption option;
            option.initFrom(this);
            pic->setPixmap(icon.pixmap(style()->pixelMetric(QStyle::PM_SmallIconSize, &option, this)));
            innerLayout->addWidget(pic);
            innerLayout->addWidget(new KSqueezedTextLabel(attachment->filename(MimeTreeParser::MessagePart::FallbackToNameOrPlaceholder)));
            innerLayout->setStretch(1, 1);

            widget->setContextMenuPolicy(Qt::CustomContextMenu);
            connect(widget, &QWidget::customContextMenuRequested, this, [parent, attachment](const QPoint &pos) {
                Q_EMIT parent->attachmentContextMenu(attachment, pos);
            });
            widgets.append(widget);
            maxWidgetWidth = qMax(maxWidgetWidth, widget->sizeHint().width());
        }
    }
    void resizeEvent(QResizeEvent *event) override
    {
        if (qAbs(width() - oldWidth) > 20) {
            doLayout();
        }
        QFrame::resizeEvent(event);
    }
    void showEvent(QShowEvent *event) override
    {
        doLayout();
        QFrame::showEvent(event);
    }
    void doLayout()
    {
        int columns = qMin(qMax(1, width() / (maxWidgetWidth + fontMetrics().horizontalAdvance(u"xx"_s))), widgets.size());
        if (grid && grid->columnCount() == columns) {
            return;
        }
        oldWidth = width();
        delete grid;
        grid = new QGridLayout(this);

        for (int i = 0; i < widgets.size(); ++i) {
            grid->addWidget(widgets[i], i / columns, i % columns);
        }
        setMinimumHeight(grid->sizeHint().height());
    }

private:
    QList<QWidget *> widgets;
    int maxWidgetWidth;
    int oldWidth;
    QGridLayout *grid;
};
}

MessageWidgetContainer::MessageWidgetContainer(const QModelIndex &idx, UrlHandler *urlHandler, QWidget *parent)
    : QFrame(parent)
    // signature
    , m_signatureInfo(idx.data(PartModel::SignatureDetailsRole).toString())
    , m_signatureSecurityLevel(idx.data(PartModel::SignatureSecurityLevelRole).value<PartModel::SecurityLevel>())
    , m_displaySignatureInfo(m_signatureSecurityLevel != PartModel::Unknow)
    , m_signatureIconName(idx.data(PartModel::SignatureIconNameRole).toString())
    // encryption
    , m_encryptionInfo(idx.data(PartModel::EncryptionDetails).value<SignatureInfo>())
    , m_encryptionSecurityLevel(idx.data(PartModel::EncryptionSecurityLevelRole).value<PartModel::SecurityLevel>())
    , m_displayEncryptionInfo(m_encryptionSecurityLevel != PartModel::Unknow)
    , m_encryptionIconName(idx.data(PartModel::EncryptionIconNameRole).toString())
    // sidebar
    , m_sidebarSecurityLevel(idx.data(PartModel::SidebarSecurityLevelRole).value<PartModel::SecurityLevel>())
    , m_urlHandler(urlHandler)
    , m_innerLayout(nullptr)
{
    createLayout(idx);
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

QLayout *MessageWidgetContainer::innerLayout() const
{
    return m_innerLayout;
}

void MessageWidgetContainer::createLayout(const QModelIndex &idx)
{
    auto vLayout = new QVBoxLayout(this);

    if (m_displayEncryptionInfo || m_displaySignatureInfo) {
        if (layoutDirection() == Qt::RightToLeft) {
            layout()->setContentsMargins(0, 0, borderWidth * 2, 0);
        } else {
            layout()->setContentsMargins(borderWidth * 2, 0, 0, 0);
        }
    } else {
        layout()->setContentsMargins({});
    }

    if (m_displayEncryptionInfo) {
        auto encryptionMessage = new KMessageWidget(this);
        encryptionMessage->setObjectName(QLatin1StringView("EncryptionMessage"));
        encryptionMessage->setCloseButtonVisible(false);
        encryptionMessage->setMessageType(getType(m_encryptionSecurityLevel));

        QString text;
        if (m_encryptionSecurityLevel == PartModel::Bad) {
            encryptionMessage->setIcon(QIcon::fromTheme(u"data-error"_s));
            if (Kleo::DeVSCompliance::isCompliant() && m_encryptionInfo.isCompliant) {
                text = i18n("This message is VS-NfD compliant encrypted but you don't have a matching secret key.", QString::fromUtf8(m_encryptionInfo.keyId));
            } else {
                text = i18n("This message is encrypted but you don't have a matching secret key.");
            }
        } else {
            encryptionMessage->setIcon(QIcon::fromTheme(u"mail-encrypted"_s));
            if (Kleo::DeVSCompliance::isCompliant() && m_encryptionInfo.isCompliant) {
                text = i18n("This message is VS-NfD compliant encrypted.");
            } else {
                text = i18n("This message is encrypted.");
            }
        }

        encryptionMessage->setText(text + QLatin1Char(' ') + u"<a href=\"messageviewer:showDetails\">Details</a>"_s);

        connect(encryptionMessage, &KMessageWidget::linkActivated, this, [this, encryptionMessage, text](const QString &link) {
            QUrl url(link);
            if (url.path() == QLatin1StringView("showDetails")) {
                QString newText = text + QLatin1Char(' ') + i18n("The message is encrypted for the following recipients:");

                newText += MimeTreeParser::decryptRecipientsToHtml(m_encryptionInfo.decryptRecipients, m_encryptionInfo.cryptoProto);

                encryptionMessage->setText(newText);
                return;
            }

            if (url.path() == QLatin1StringView("showCertificate")) {
                m_urlHandler->handleClick(QUrl(link), window()->windowHandle());
            }
        });

        vLayout->addWidget(encryptionMessage);
    }

    if (m_displaySignatureInfo) {
        auto signatureMessage = new KMessageWidget(this);
        signatureMessage->setObjectName(u"SignatureMessage"_s);
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

    // Mail contents to be inserted, here...
    m_innerLayout = new QVBoxLayout;
    m_innerLayout->setContentsMargins({});
    vLayout->addLayout(m_innerLayout);

    const auto attachments = idx.data(PartModel::OwnedAttachmentsRole).value<QList<QSharedPointer<MimeTreeParser::MessagePart>>>();
    if (!attachments.isEmpty()) {
        vLayout->addWidget(new AttachmentBox(attachments, this));
    }
}

#include "moc_messagecontainerwidget_p.cpp"

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
    AttachmentBox(const QList<QSharedPointer<MimeTreeParser::Core::MessagePart>> &attachments, MessageWidgetContainer *parent)
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
            innerLayout->addWidget(new KSqueezedTextLabel(attachment->filename(MimeTreeParser::Core::MessagePart::FallbackToNameOrPlaceholder)));
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
    , m_containerPart(static_cast<const PartModel *>(idx.model())->part(idx).get())
    // signature
    , m_signatureInfo(idx.data(PartModel::SignatureInfoRole).value<GenericInfo>())
    , m_displaySignatureInfo(m_signatureInfo.securityLevel != PartModel::Unknow)
    // encryption
    , m_encryptionInfo(idx.data(PartModel::EncryptionInfoRole).value<GenericInfo>())
    , m_displayEncryptionInfo(m_encryptionInfo.securityLevel != PartModel::Unknow)
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

KMessageWidget *MessageWidgetContainer::makeInfoBox(QWidget *parent, const GenericInfo &info, const UrlHandler *urlHandler)
{
    auto box = new KMessageWidget(parent);
    box->setCloseButtonVisible(false);
    box->setMessageType(getType(info.securityLevel));
    box->setWordWrap(true);

    QString text = info.summary;
    box->setIcon(QIcon::fromTheme(info.iconName));
    if (!info.details.isEmpty()) {
        text += QLatin1Char(' ') + u"<a href=\"messageviewer:showDetails\">Details</a>"_s;
    }
    box->setText(text);

    connect(box, &KMessageWidget::linkActivated, parent, [parent, box, info, urlHandler](const QString &link) {
        QUrl url(link);
        if (url.path() == QLatin1StringView("showDetails")) {
            box->setText(info.summary + u' ' + info.details.join(u' '));
            return;
        }
        urlHandler->handleClick(QUrl(link), parent->window()->windowHandle());
    });

    return box;
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
        auto encryptionMessage = makeInfoBox(this, m_encryptionInfo, m_urlHandler);
        encryptionMessage->setObjectName(QLatin1StringView("EncryptionMessage"));
        vLayout->addWidget(encryptionMessage);
    }

    if (m_displaySignatureInfo) {
        auto signatureMessage = makeInfoBox(this, m_signatureInfo, m_urlHandler);
        signatureMessage->setObjectName(u"SignatureMessage"_s);
        vLayout->addWidget(signatureMessage);
    }

    // Mail contents to be inserted, here...
    m_innerLayout = new QVBoxLayout;
    m_innerLayout->setContentsMargins({});
    vLayout->addLayout(m_innerLayout);

    const auto attachments = idx.data(PartModel::AssociatedAttachmentsRole).value<QList<QSharedPointer<MimeTreeParser::Core::MessagePart>>>();
    if (!attachments.isEmpty()) {
        vLayout->addWidget(new AttachmentBox(attachments, this));
    }
}

#include "moc_messagecontainerwidget_p.cpp"

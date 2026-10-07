// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "../core/utils.h"

#include "messagecontainerwidget_p.h"
#include "partmodel.h"
#include <MimeTreeParserCore/UrlHandler>

#include <KLocalizedString>
#include <KSqueezedTextLabel>
#include <Libkleo/Compliance>
#include <Libkleo/Formatting>
#include <QGpgME/Protocol>

#include <QLabel>
#include <QMimeDatabase>
#include <QPaintEvent>
#include <QPainter>
#include <QPushButton>
#include <QStyleOption>
#include <QVBoxLayout>

#include <gpgme++/verificationresult.h>

using namespace Qt::Literals::StringLiterals;

namespace
{

const int sideBorderWidth = 5;
const int boxBorderWidth = 2;

QColor getColor(PartModel::SecurityLevel securityLevel)
{
    const static QHash<PartModel::SecurityLevel, QColor> colors{
        {PartModel::Good, QColor(39, 174, 96)}, // Window: ForegroundPositive
        {PartModel::Bad, QColor(218, 68, 83)}, // Window: ForegroundNegative
        {PartModel::NotSoGood, QColor(246, 116, 0)}, // Window: ForegroundNeutral
    };

    return colors.value(securityLevel, QColor());
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

/** Box to display information on encryption/signature. This is essentially a stripped down KMessageWidget.
 *  We do not use a regular KMessageWidget, because there, we cannot insert a "Show Details"-button without
 *  wasting a lot of space (would be inserted on a row of its own). */
class CryptoStatusBox : public QWidget
{
    Q_OBJECT
public:
    CryptoStatusBox(const GenericInfo &info, QWidget *parent)
        : QWidget(parent)
    {
        mSummaryLabel.setWordWrap(true);
        mSummaryLabel.setTextInteractionFlags(Qt::TextBrowserInteraction | Qt::TextSelectableByKeyboard);
        mSummaryLabel.setObjectName("SummaryLabel");
        // Taken form KMessageWidget: Make sure calling setFocus() sets a sensible item. This is useful for accessibility, because when the focus
        // is moved to the textLabel, screen readers will first announce the accessible name of the messageWidget e.g. "Error" and
        // then the textLabel's text.
        setFocusProxy(&mSummaryLabel);
        connect(&mSummaryLabel, &QLabel::linkActivated, this, &CryptoStatusBox::linkActivated);
        mDetailsLabel.setWordWrap(true);
        mDetailsLabel.setTextInteractionFlags(Qt::TextBrowserInteraction | Qt::TextSelectableByKeyboard);
        mDetailsLabel.setObjectName("DetailsLabel");
        connect(&mDetailsLabel, &QLabel::linkActivated, this, &CryptoStatusBox::linkActivated);
        mDetailsButton.setObjectName("DetailsButton");
        connect(&mDetailsButton, &QPushButton::clicked, this, &CryptoStatusBox::toggleDetails);

        auto grid = new QGridLayout(this);
        grid->addWidget(&mIconLabel, 0, 0);
        grid->addWidget(&mSummaryLabel, 0, 1);
        grid->addWidget(&mDetailsButton, 0, 2, 1, 1, Qt::AlignTop);
        grid->addWidget(&mDetailsLabel, 1, 1);
        grid->setColumnStretch(1, 2);

        setMessage(info);
    }
    void setMessage(const GenericInfo &info)
    {
        mSummaryLabel.setText(info.summary);
        if (mDetailsLabel.isVisibleTo(this)) {
            toggleDetails();
        }
        mDetailsLabel.setText(info.details.isEmpty() ? QString() : (u"<p>"_s + info.details.join(u"</p><p>"_s) + u"</p>"_s));
        mDetailsButton.setVisible(!info.details.isEmpty());
        mIconLabel.setPixmap(QIcon::fromTheme(info.iconName).pixmap(style()->pixelMetric(QStyle::PM_ToolBarIconSize)));

        switch (info.securityLevel) {
        case PartModel::SecurityLevel::Bad:
            setAccessibleName(i18nc("accessible name", "Crypto errors"));
            break;
        case PartModel::SecurityLevel::NotSoGood:
            setAccessibleName(i18nc("accessible name", "Crypto summary and warnings"));
            break;
        case PartModel::SecurityLevel::Good:
            setAccessibleName(i18nc("accessible name", "Crypto summary"));
            break;
        default:
            setAccessibleName(QString());
        }
        auto p = palette();
        p.setColor(QPalette::Window, getColor(info.securityLevel));
        setPalette(p);
    }
    void toggleDetails()
    {
        if (mDetailsLabel.isVisibleTo(this)) {
            mDetailsLabel.setVisible(false);
            mDetailsButton.setText(i18nc("@button", "Show Details"));
        } else {
            mDetailsLabel.setVisible(true);
            mDetailsButton.setText(i18nc("@button", "Hide Details"));
            // Is this enough to make screen readers read the new message?
            mDetailsLabel.setFocus();
        }
    }
Q_SIGNALS:
    void linkActivated(const QString &link);

private:
    void paintEvent(QPaintEvent *event) override
    {
        // paintEvent() essentially copied from KMessageWidget
        Q_UNUSED(event)
        QPainter painter(this);
        constexpr float radius = 4 * 0.6;
        const QRect innerRect = rect().marginsRemoved(QMargins() + boxBorderWidth / 2);
        const QColor color = palette().color(QPalette::Window);
        constexpr float alpha = 0.2;
        const QColor parentWindowColor = (parentWidget() ? parentWidget()->palette() : qApp->palette()).color(QPalette::Window);
        const int newRed = (color.red() * alpha) + (parentWindowColor.red() * (1 - alpha));
        const int newGreen = (color.green() * alpha) + (parentWindowColor.green() * (1 - alpha));
        const int newBlue = (color.blue() * alpha) + (parentWindowColor.blue() * (1 - alpha));

        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(QColor(newRed, newGreen, newBlue));
        painter.setPen(QPen(color, boxBorderWidth));
        painter.drawRoundedRect(innerRect, radius, radius);
    }

    QLabel mDetailsLabel;
    QLabel mSummaryLabel;
    QLabel mIconLabel;
    QPushButton mDetailsButton;
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
    , m_sidebarSecurityLevel(qMax(m_signatureInfo.securityLevel, m_encryptionInfo.securityLevel))
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
        r.setX(width() - sideBorderWidth);
        r.setWidth(sideBorderWidth);
        const QColor color = getColor(m_sidebarSecurityLevel);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(QColor(color));
        painter.setPen(QPen(Qt::NoPen));
        painter.drawRect(r);
    } else {
        auto r = rect();
        r.setWidth(sideBorderWidth);
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

QWidget *MessageWidgetContainer::makeInfoBox(QWidget *parent, const GenericInfo &info, UrlHandler *urlHandler)
{
    auto box = new CryptoStatusBox(info, parent);
    connect(box, &CryptoStatusBox::linkActivated, parent, [parent, urlHandler](const QString &link) {
        urlHandler->handleClick(QUrl(link), parent->window()->windowHandle());
    });
    return box;
}

void MessageWidgetContainer::createLayout(const QModelIndex &idx)
{
    auto vLayout = new QVBoxLayout(this);

    if (m_displayEncryptionInfo || m_displaySignatureInfo) {
        if (layoutDirection() == Qt::RightToLeft) {
            layout()->setContentsMargins(0, 0, sideBorderWidth * 2, 0);
        } else {
            layout()->setContentsMargins(sideBorderWidth * 2, 0, 0, 0);
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

#include "messagecontainerwidget.moc"

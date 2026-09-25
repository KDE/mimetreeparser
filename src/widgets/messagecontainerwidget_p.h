// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include <MimeTreeParserCore/PartModel>
#include <QFrame>
#include <QPointer>

class QPaintEvent;
class UrlHandler;
class KMessageWidget;

/// \internal
class MessageWidgetContainer : public QFrame
{
    Q_OBJECT

public:
    /*!
     * \brief Constructs a MessageWidgetContainer
     * \param part The containing part
     * \param urlHandler The URL handler for handling links
     * \param parent The parent widget
     */
    explicit MessageWidgetContainer(const QModelIndex &idx, UrlHandler *urlHandler, QWidget *parent = nullptr);
    /*!
     * \brief Destroys the MessageWidgetContainer
     */
    ~MessageWidgetContainer() override;
    QLayout *innerLayout() const;
    const MimeTreeParser::Core::MessagePart *containerPart() const
    {
        return m_containerPart;
    }

    static KMessageWidget *makeInfoBox(QWidget *parent, const GenericInfo &info, const UrlHandler *urlHandler);

Q_SIGNALS:
    void attachmentContextMenu(const QSharedPointer<MimeTreeParser::Core::MessagePart> part, const QPoint &pos);

protected:
    /*!
     * \brief Handles paint events to draw the container
     * \param event The paint event
     */
    void paintEvent(QPaintEvent *event) override;

private:
    void createLayout(const QModelIndex &idx);
    QPointer<const MimeTreeParser::Core::MessagePart> m_containerPart;

    GenericInfo const m_signatureInfo;
    bool m_displaySignatureInfo;

    GenericInfo const m_encryptionInfo;
    bool m_displayEncryptionInfo;

    PartModel::SecurityLevel m_sidebarSecurityLevel;

    UrlHandler *const m_urlHandler;
    QLayout *m_innerLayout;
};

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
    MIMETREEPARSER_WIDGETS_NO_EXPORT QLayout *innerLayout() const;

Q_SIGNALS:
    MIMETREEPARSER_WIDGETS_NO_EXPORT void attachmentContextMenu(const QSharedPointer<MimeTreeParser::MessagePart> part, const QPoint &pos);

protected:
    /*!
     * \brief Handles paint events to draw the container
     * \param event The paint event
     */
    void paintEvent(QPaintEvent *event) override;

private:
    MIMETREEPARSER_WIDGETS_NO_EXPORT void createLayout(const QModelIndex &idx);

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
    QLayout *m_innerLayout;
};

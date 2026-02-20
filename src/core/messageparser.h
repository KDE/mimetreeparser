// SPDX-FileCopyrightText: 2016 Christian Mollekopf <mollekopf@kolabsys.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once
#include <QObject>
#include <QString>

#include <KMime/Message>
#include <QAbstractItemModel>

#include "mimetreeparser_core_export.h"
#include "partmodel.h"
#include <memory>

class MessagePartPrivate;
class AttachmentModel;

class MIMETREEPARSER_CORE_EXPORT MessageParser : public QObject
{
    Q_OBJECT
    Q_PROPERTY(std::shared_ptr<KMime::Message> message READ message WRITE setMessage NOTIFY htmlChanged)
    Q_PROPERTY(PartModel *parts READ parts NOTIFY htmlChanged)
    Q_PROPERTY(QAbstractItemModel *attachments READ attachments NOTIFY htmlChanged)
    Q_PROPERTY(QString structureAsString READ structureAsString NOTIFY htmlChanged)
    Q_PROPERTY(bool loaded READ loaded NOTIFY htmlChanged)

    Q_PROPERTY(QString subject READ subject NOTIFY htmlChanged)
    Q_PROPERTY(QString from READ from NOTIFY htmlChanged)
    Q_PROPERTY(QString sender READ sender NOTIFY htmlChanged)
    Q_PROPERTY(QString to READ to NOTIFY htmlChanged)
    Q_PROPERTY(QString cc READ cc NOTIFY htmlChanged)
    Q_PROPERTY(QString bcc READ bcc NOTIFY htmlChanged)
    Q_PROPERTY(QDateTime date READ date NOTIFY htmlChanged)

public:
    /*!
     * \brief Constructs a MessageParser
     * \param parent The parent object
     */
    explicit MessageParser(QObject *parent = nullptr);
    /*!
     * \brief Destroys the MessageParser
     */
    ~MessageParser();

    /*!
     * \brief Returns the current message
     * \return A shared pointer to the message
     */
    [[nodiscard]] std::shared_ptr<KMime::Message> message() const;
    /*!
     * \brief Sets the message to parse
     * \param message The message to parse
     */
    void setMessage(const std::shared_ptr<KMime::Message> message);
    /*!
     * \brief Returns the part model
     * \return A pointer to the parts model
     */
    [[nodiscard]] PartModel *parts() const;
    /*!
     * \brief Returns the attachment model
     * \return A pointer to the attachments model
     */
    [[nodiscard]] AttachmentModel *attachments() const;
    /*!
     * \brief Returns the message structure as a string
     * \return A string representation of the message structure
     */
    [[nodiscard]] QString structureAsString() const;
    /*!
     * \brief Returns whether the message has been loaded and parsed
     * \return True if the message is loaded, false otherwise
     */
    [[nodiscard]] bool loaded() const;

    /*!
     * \brief Returns the message subject
     * \return The subject line
     */
    [[nodiscard]] QString subject() const;
    /*!
     * \brief Returns the message From header
     * \return The from address
     */
    [[nodiscard]] QString from() const;
    /*!
     * \brief Returns the message Sender header
     * \return The sender address
     */
    [[nodiscard]] QString sender() const;
    /*!
     * \brief Returns the message To header
     * \return The recipient addresses
     */
    [[nodiscard]] QString to() const;
    /*!
     * \brief Returns the message Cc header
     * \return The carbon copy addresses
     */
    [[nodiscard]] QString cc() const;
    /*!
     * \brief Returns the message Bcc header
     * \return The blind carbon copy addresses
     */
    [[nodiscard]] QString bcc() const;
    /*!
     * \brief Returns the message date
     * \return The date the message was sent
     */
    [[nodiscard]] QDateTime date() const;

Q_SIGNALS:
    /*!
     * \brief Emitted when the message or its HTML content has changed
     */
    void htmlChanged();

private:
    std::unique_ptr<MessagePartPrivate> d;
};

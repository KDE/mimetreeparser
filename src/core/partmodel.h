// SPDX-FileCopyrightText: 2016 Christian Mollekopf <mollekopf@kolabsys.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include "messagepart.h"
#include "mimetreeparser_core_export.h"

#include <QAbstractItemModel>
#include <QModelIndex>

#include <gpgme++/decryptionresult.h>
#include <gpgme++/key.h>
#include <gpgme++/verificationresult.h>

#include <memory>

namespace QGpgME
{
class Protocol;
}

namespace MimeTreeParser
{
class ObjectTreeParser;
}
class PartModelPrivate;
/*!
 * \class MimeTreeParser::PartModel
 * \inmodule MimeTreeParserCore
 * \inheaderfile MimeTreeParserCore/PartModel
 */
class MIMETREEPARSER_CORE_EXPORT PartModel : public QAbstractItemModel
{
    Q_OBJECT
    Q_PROPERTY(bool showHtml READ showHtml WRITE setShowHtml NOTIFY showHtmlChanged)
    Q_PROPERTY(bool containsHtml READ containsHtml NOTIFY containsHtmlChanged)
    Q_PROPERTY(bool trimMail READ trimMail WRITE setTrimMail NOTIFY trimMailChanged)
    Q_PROPERTY(bool isTrimmed READ isTrimmed NOTIFY trimMailChanged)
public:
    /*!
     * \brief Constructs a PartModel
     * \param parser The object tree parser
     */
    explicit PartModel(std::shared_ptr<MimeTreeParser::ObjectTreeParser> parser);
    /*!
     * \brief Destroys the PartModel
     */
    ~PartModel() override;

    static std::pair<QString, bool> trim(const QString &text);

public:
    enum class Types : quint8 {
        Error,
        Encapsulated,
        Ical,
        Plain,
        None,
        Html,
    };
    Q_ENUM(Types)

    enum Roles {
        TypeRole = Qt::UserRole + 1,
        ContentRole,
        IsEmbeddedRole,
        IsErrorRole,
        SidebarSecurityLevelRole,
        EncryptionSecurityLevelRole,
        EncryptionIconNameRole,
        SignatureSecurityLevelRole,
        SignatureDetailsRole,
        SignatureIconNameRole,
        EncryptionDetails,
        ErrorType,
        ErrorString,
        SenderRole,
        DateRole,
    };

    /// This enum maps directly to color displayed in the UI for the following elements:
    /// - Encryption info box
    /// - Signature info box
    /// - Sidebar (worse of the two above)
    enum SecurityLevel {
        Unknow, ///< Do not display element (not encrypted or not signed)
        Good, ///< Green
        NotSoGood, ///< Orange
        Bad, ////< Red
    };
    Q_ENUM(SecurityLevel)

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
    [[nodiscard]] QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QModelIndex parent(const QModelIndex &index) const override;
    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    /*!
     * \brief Sets whether to display HTML content
     * \param html True to display HTML, false for plaintext
     */
    void setShowHtml(bool html);
    /*!
     * \brief Returns whether HTML content is displayed
     * \return True if HTML is shown
     */
    [[nodiscard]] bool showHtml() const;
    /*!
     * \brief Returns whether the message contains HTML content
     * \return True if HTML content is present
     */
    [[nodiscard]] bool containsHtml() const;

    /*!
     * \brief Sets whether to trim mail content
     * \param trim True to trim mail content
     */
    void setTrimMail(bool trim);
    /*!
     * \brief Returns whether mail content is trimmed
     * \return True if mail trimming is enabled
     */
    [[nodiscard]] bool trimMail() const;
    /*!
     * \brief Returns whether the current content is trimmed
     * \return True if the content has been trimmed
     */
    [[nodiscard]] bool isTrimmed() const;

    /*!
     * \brief Returns the security level of a signature
     * \param messagePart The message part to check
     * \return The signature security level
     */
    static SecurityLevel signatureSecurityLevel(MimeTreeParser::MessagePart *messagePart);
    /*!
     * \brief Returns the signature details for a message part
     * \param messagePart The message part to check
     * \return A string with signature details
     */
    static QString signatureDetails(MimeTreeParser::MessagePart *messagePart);

Q_SIGNALS:
    /*!
     * \brief Emitted when the HTML display preference changes
     */
    void showHtmlChanged();
    /*!
     * \brief Emitted when the mail trimming preference changes
     */
    void trimMailChanged();
    /*!
     * \brief Emitted when the message content changes with respect to HTML presence
     */
    void containsHtmlChanged();

private:
    std::unique_ptr<PartModelPrivate> d;
};

/*!
 * \class MimeTreeParser::SignatureInfo
 * \inmodule MimeTreeParserCore
 * \inheaderfile MimeTreeParserCore/PartModel
 * \brief Contains information about a message signature
 */
class MIMETREEPARSER_CORE_EXPORT SignatureInfo
{
    Q_GADGET
    Q_PROPERTY(QByteArray keyId MEMBER keyId CONSTANT)
    Q_PROPERTY(bool keyMissing MEMBER keyMissing CONSTANT)
    Q_PROPERTY(bool keyRevoked MEMBER keyRevoked CONSTANT)
    Q_PROPERTY(bool keyExpired MEMBER keyExpired CONSTANT)
    Q_PROPERTY(bool sigExpired MEMBER sigExpired CONSTANT)
    Q_PROPERTY(bool crlMissing MEMBER crlMissing CONSTANT)
    Q_PROPERTY(bool crlTooOld MEMBER crlTooOld CONSTANT)

    Q_PROPERTY(QString signer MEMBER signer CONSTANT)
    Q_PROPERTY(QStringList signerMailAddresses MEMBER signerMailAddresses CONSTANT)
    Q_PROPERTY(bool signatureIsGood MEMBER signatureIsGood CONSTANT)
    Q_PROPERTY(bool isCompliant MEMBER isCompliant CONSTANT)

    /// Validity information of the key who signed the message.
    Q_PROPERTY(QString keyTrust MEMBER keyTrust CONSTANT)

    Q_PROPERTY(GpgME::Signature::Summary signatureSummary MEMBER signatureSummary CONSTANT)

public:
    bool keyRevoked = false;
    bool keyExpired = false;
    bool sigExpired = false;
    bool keyMissing = false;
    bool crlMissing = false;
    bool crlTooOld = false;
    bool isCompliant = false;
    QString keyTrust;
    QByteArray keyId;
    const QGpgME::Protocol *cryptoProto = nullptr;
    std::vector<std::pair<GpgME::DecryptionResult::Recipient, GpgME::Key>> decryptRecipients;

    QString signer;
    GpgME::Signature::Summary signatureSummary;
    QStringList signerMailAddresses;
    bool signatureIsGood = false;
};

// SPDX-FileCopyrightText: 2015 Sandro Knauß <knauss@kolabsys.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include "enums.h"
#include "mimetreeparser_core_export.h"
#include "partmetadata.h"

#include <gpgme++/decryptionresult.h>
#include <gpgme++/importresult.h>
#include <gpgme++/verificationresult.h>

#include <KMime/Message>

#include <QMap>
#include <QSharedPointer>
#include <QString>

namespace KMime
{
class Content;
}

namespace QGpgME
{
class Protocol;
}

namespace MimeTreeParser
{

class ObjectTreeParser;
class MultiPartAlternativeBodyPartFormatter;

class SignedMessagePart;
class EncryptedMessagePart;
/*!
 * \class MimeTreeParser::MessagePart
 * \inmodule MimeTreeParserCore
 * \inheaderfile MimeTreeParserCore/MessagePart
 */
class MIMETREEPARSER_CORE_EXPORT MessagePart : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool attachment READ isAttachment CONSTANT)
    Q_PROPERTY(bool root READ isRoot CONSTANT)
    Q_PROPERTY(bool isHtml READ isHtml CONSTANT)
    Q_PROPERTY(QString plaintextContent READ plaintextContent CONSTANT)
    Q_PROPERTY(QString htmlContent READ htmlContent CONSTANT)
public:
    enum Disposition {
        Inline,
        Attachment,
        Invalid,
    };
    /*!
     * \brief Constructs a MessagePart
     * \param otp The object tree parser
     * \param text The text content of the part
     * \param node The MIME content node
     */
    MessagePart(ObjectTreeParser *otp, const QString &text, KMime::Content *node = nullptr);

    /*!
     * \brief Destroys the MessagePart
     */
    ~MessagePart() override;

    /*!
     * \brief Returns the text content of this part
     * \return The text content
     */
    [[nodiscard]] virtual QString text() const;
    /*!
     * \brief Sets the text content of this part
     * \param text The text to set
     */
    void setText(const QString &text);
    /*!
     * \brief Returns whether this part is an attachment
     * \return True if this is an attachment, false otherwise
     */
    [[nodiscard]] virtual bool isAttachment() const;

    /*!
     * \brief Sets whether this is the root message part
     * \param root True if this is the root part
     */
    void setIsRoot(bool root);
    /*!
     * \brief Returns whether this is the root message part
     * \return True if this is the root part
     */
    [[nodiscard]] bool isRoot() const;

    /*!
     * \brief Sets the parent message part
     * \param parentPart The parent message part
     */
    void setParentPart(MessagePart *parentPart);
    /*!
     * \brief Returns the parent message part
     * \return A pointer to the parent part, or nullptr if this is the root
     */
    [[nodiscard]] MessagePart *parentPart() const;

    /*!
     * \brief Returns the plaintext content of this part
     * \return The plaintext content
     */
    [[nodiscard]] virtual QString plaintextContent() const;
    /*!
     * \brief Returns the HTML content of this part
     * \return The HTML content
     */
    [[nodiscard]] virtual QString htmlContent() const;

    /*!
     * \brief Returns whether this part is in HTML format
     * \return True if this is HTML content, false otherwise
     */
    [[nodiscard]] virtual bool isHtml() const;

    /*!
     * \brief Returns the MIME type of this part
     * \return The MIME type
     */
    [[nodiscard]] QByteArray mimeType() const;
    /*!
     * \brief Returns the character set of this part
     * \return The charset name
     */
    [[nodiscard]] QByteArray charset() const;
    /*!
     * \brief Returns the filename of this attachment
     * \return The filename, or empty string if not an attachment
     */
    [[nodiscard]] QString filename() const;
    /*!
     * \brief Returns the disposition of this part
     * \return The disposition (Inline, Attachment, or Invalid)
     */
    [[nodiscard]] Disposition disposition() const;
    /*!
     * \brief Returns whether this part is text content
     * \return True if this is text content, false otherwise
     */
    [[nodiscard]] bool isText() const;

    enum Error {
        NoError = 0,
        PassphraseError,
        NoKeyError,
        UnknownError,
    };

    /*!
     * \brief Returns the error state of this part
     * \return The error code
     */
    [[nodiscard]] Error error() const;
    /*!
     * \brief Returns the error message
     * \return A human-readable error description
     */
    [[nodiscard]] QString errorString() const;

    /*!
     * \brief Returns the metadata of this part
     * \return A pointer to the part metadata
     */
    [[nodiscard]] PartMetaData *partMetaData();

    /*!
     * \brief Appends a sub-part to this part
     * \param messagePart The message part to append
     */
    void appendSubPart(const QSharedPointer<MessagePart> &messagePart);
    /*!
     * \brief Returns all sub-parts of this part
     * \return A list of sub-parts
     */
    [[nodiscard]] const QList<QSharedPointer<MessagePart>> &subParts() const;
    /*!
     * \brief Returns whether this part has sub-parts
     * \return True if there are sub-parts, false otherwise
     */
    [[nodiscard]] bool hasSubParts() const;

    /*!
     * \brief Returns the MIME content node for this part
     * \return A pointer to the KMime::Content node, or nullptr if not set
     */
    KMime::Content *node() const;

    /*!
     * \brief Returns the signature state of this part
     * \return The signature state
     */
    virtual KMMsgSignatureState signatureState() const;
    /*!
     * \brief Returns the encryption state of this part
     * \return The encryption state
     */
    virtual KMMsgEncryptionState encryptionState() const;

    /*!
     * \brief Returns all signature parts of this message
     * \return A list of SignedMessagePart pointers
     */
    [[nodiscard]] QList<SignedMessagePart *> signatures() const;
    /*!
     * \brief Returns all encrypted parts of this message
     * \return A list of EncryptedMessagePart pointers
     */
    [[nodiscard]] QList<EncryptedMessagePart *> encryptions() const;

    /*!
     * Retrieve the header @header in this part or any parent parent.
     *
     * Useful for MemoryHole support.
     */
    [[nodiscard]] KMime::Headers::Base *header(const char *header) const;

    /*!
     * \brief Binds the lifetime of this part to a MIME content node
     * \param node The KMime::Content node to bind to
     */
    void bindLifetime(KMime::Content *);

protected:
    /*!
     * \brief Parses a MIME part from a KMime::Content node
     * \param node The content node to parse
     * \param onlyOneMimePart If true, only parse one MIME part
     */
    void parseInternal(KMime::Content *node, bool onlyOneMimePart = false);
    /*!
     * \brief Parses a MIME message from raw data
     * \param data The MIME message data
     */
    void parseInternal(const QByteArray &data);
    /*!
     * \brief Renders the internal text representation of this part
     * \return The rendered text
     */
    [[nodiscard]] QString renderInternalText() const;

    QString mText;
    ObjectTreeParser *mOtp;
    PartMetaData mMetaData;
    MessagePart *mParentPart;
    KMime::Content *mNode;
    QList<KMime::Content *> mNodesToDelete;
    Error mError;

private:
    QList<QSharedPointer<MessagePart>> mBlocks;
    bool mRoot;
};

class MIMETREEPARSER_CORE_EXPORT MimeMessagePart : public MessagePart
{
    Q_OBJECT
public:
    /*!
     * \brief Constructs a MimeMessagePart
     * \param otp The object tree parser
     * \param node The MIME content node
     * \param onlyOneMimePart If true, only parse one MIME part
     */
    MimeMessagePart(MimeTreeParser::ObjectTreeParser *otp, KMime::Content *node, bool onlyOneMimePart = false);
    /*!
     * \brief Destroys the MimeMessagePart
     */
    ~MimeMessagePart() override;

    /*!
     * \brief Returns the MIME part as text
     * \return The MIME content as text
     */
    [[nodiscard]] QString text() const override;

    /*!
     * \brief Returns the plaintext content of the MIME part
     * \return The plaintext representation
     */
    [[nodiscard]] QString plaintextContent() const override;
    /*!
     * \brief Returns the HTML content of the MIME part
     * \return The HTML representation
     */
    [[nodiscard]] QString htmlContent() const override;

private:
    friend class AlternativeMessagePart;
};

class MIMETREEPARSER_CORE_EXPORT MessagePartList : public MessagePart
{
    Q_OBJECT
public:
    /*!
     * \brief Constructs a MessagePartList
     * \param otp The object tree parser
     * \param node The MIME content node
     */
    MessagePartList(MimeTreeParser::ObjectTreeParser *otp, KMime::Content *node);
    /*!
     * \brief Destroys the MessagePartList
     */
    ~MessagePartList() override = default;

    /*!
     * \brief Returns the text content of all sub-parts combined
     * \return The combined text content
     */
    [[nodiscard]] QString text() const override;

    /*!
     * \brief Returns the plaintext content of all sub-parts combined
     * \return The combined plaintext representation
     */
    [[nodiscard]] QString plaintextContent() const override;
    [[nodiscard]] QString htmlContent() const override;
};

class MIMETREEPARSER_CORE_EXPORT TextMessagePart : public MessagePartList
{
    Q_OBJECT
public:
    /*!
     * \brief Constructs a TextMessagePart
     * \param otp The object tree parser
     * \param node The MIME content node
     */
    TextMessagePart(MimeTreeParser::ObjectTreeParser *otp, KMime::Content *node);
    /*!
     * \brief Destroys the TextMessagePart
     */
    ~TextMessagePart() override = default;

    /*!
     * \brief Returns the signature state of the text content
     * \return The signature state
     */
    [[nodiscard]] KMMsgSignatureState signatureState() const override;
    /*!
     * \brief Returns the encryption state of the text content
     * \return The encryption state
     */
    [[nodiscard]] KMMsgEncryptionState encryptionState() const override;

private:
    MIMETREEPARSER_CORE_NO_EXPORT void parseContent();

    KMMsgSignatureState mSignatureState;
    KMMsgEncryptionState mEncryptionState;

    friend class ObjectTreeParser;
};

class MIMETREEPARSER_CORE_EXPORT AttachmentMessagePart : public TextMessagePart
{
    Q_OBJECT
public:
    /*!
     * \brief Constructs an AttachmentMessagePart
     * \param otp The object tree parser
     * \param node The MIME content node
     */
    AttachmentMessagePart(MimeTreeParser::ObjectTreeParser *otp, KMime::Content *node);
    /*!
     * \brief Destroys the AttachmentMessagePart
     */
    ~AttachmentMessagePart() override = default;
    /*!
     * \brief Returns whether this is an attachment
     * \return Always returns true for AttachmentMessagePart
     */
    [[nodiscard]] virtual bool isAttachment() const override
    {
        return true;
    }
};

class MIMETREEPARSER_CORE_EXPORT HtmlMessagePart : public MessagePart
{
    Q_OBJECT
public:
    /*!
     * \brief Constructs an HtmlMessagePart
     * \param otp The object tree parser
     * \param node The MIME content node
     */
    HtmlMessagePart(MimeTreeParser::ObjectTreeParser *otp, KMime::Content *node);

    /*!
     * \brief Destroys the HtmlMessagePart
     */
    ~HtmlMessagePart() override = default;
    /*!
     * \brief Returns whether this is HTML content
     * \return Always returns true for HtmlMessagePart
     */
    [[nodiscard]] bool isHtml() const override
    {
        return true;
    }
};

class MIMETREEPARSER_CORE_EXPORT AlternativeMessagePart : public MessagePart
{
    Q_OBJECT
public:
    enum HtmlMode {
        Normal, ///< A normal plaintext message, non-multipart
        Html, ///< A HTML message, non-multipart
        MultipartPlain, ///< A multipart/alternative message, the plain text part is currently displayed
        MultipartHtml, ///< A multipart/altervative message, the HTML part is currently displayed
        MultipartIcal ///< A multipart/altervative message, the ICal part is currently displayed
    };

    /*!
     * \brief Constructs an AlternativeMessagePart
     * \param otp The object tree parser
     * \param node The MIME content node
     */
    AlternativeMessagePart(MimeTreeParser::ObjectTreeParser *otp, KMime::Content *node);
    /*!
     * \brief Destroys the AlternativeMessagePart
     */
    ~AlternativeMessagePart() override;

    /*!
     * \brief Returns the text content of the alternative part
     * \return The selected alternative text
     */
    [[nodiscard]] QString text() const override;

    /*!
     * \brief Returns whether the selected alternative is HTML
     * \return True if the selected alternative is HTML
     */
    [[nodiscard]] bool isHtml() const override;

    /*!
     * \brief Returns the plaintext alternative content
     * \return The plaintext representation
     */
    [[nodiscard]] QString plaintextContent() const override;
    /*!
     * \brief Returns the HTML alternative content
     * \return The HTML representation
     */
    [[nodiscard]] QString htmlContent() const override;
    /*!
     * \brief Returns the iCalendar alternative content
     * \return The iCalendar representation
     */
    [[nodiscard]] QString icalContent() const;

    /*!
     * \brief Returns the list of available alternative modes
     * \return A list of available HtmlMode values
     */
    [[nodiscard]] QList<HtmlMode> availableModes();

private:
    QMap<HtmlMode, QSharedPointer<MessagePart>> mChildParts;

    friend class ObjectTreeParser;
    friend class MultiPartAlternativeBodyPartFormatter;
};

class MIMETREEPARSER_CORE_EXPORT CertMessagePart : public MessagePart
{
    Q_OBJECT
public:
    /*!
     * \brief Constructs a CertMessagePart
     * \param otp The object tree parser
     * \param node The MIME content node
     * \param cryptoProto The cryptography protocol handler
     */
    CertMessagePart(MimeTreeParser::ObjectTreeParser *otp, KMime::Content *node, QGpgME::Protocol *cryptoProto);
    /*!
     * \brief Destroys the CertMessagePart
     */
    ~CertMessagePart() override;

    /*!
     * \brief Returns the certificate information as text
     * \return The certificate text representation
     */
    [[nodiscard]] QString text() const override;

private:
    const QGpgME::Protocol *mCryptoProto;
    GpgME::ImportResult mInportResult;
};

class MIMETREEPARSER_CORE_EXPORT EncapsulatedRfc822MessagePart : public MessagePart
{
    Q_OBJECT
public:
    /*!
     * \brief Constructs an EncapsulatedRfc822MessagePart
     * \param otp The object tree parser
     * \param node The MIME content node
     * \param message The RFC 822 message
     */
    EncapsulatedRfc822MessagePart(MimeTreeParser::ObjectTreeParser *otp, KMime::Content *node, const std::shared_ptr<KMime::Message> &message);
    /*!
     * \brief Destroys the EncapsulatedRfc822MessagePart
     */
    ~EncapsulatedRfc822MessagePart() override = default;

    /*!
     * \brief Returns the message text
     * \return The message text
     */
    [[nodiscard]] QString text() const override;
    /*!
     * \brief Returns the From header of the encapsulated message
     * \return The From address
     */
    [[nodiscard]] QString from() const;
    /*!
     * \brief Returns the date of the encapsulated message
     * \return The message date
     */
    [[nodiscard]] QDateTime date() const;

private:
    const std::shared_ptr<KMime::Message> mMessage;
};

class MIMETREEPARSER_CORE_EXPORT EncryptedMessagePart : public MessagePart
{
    Q_OBJECT
    Q_PROPERTY(bool decryptMessage READ decryptMessage WRITE setDecryptMessage)
    Q_PROPERTY(bool isEncrypted READ isEncrypted)
    Q_PROPERTY(bool isNoSecKey READ isNoSecKey)
    Q_PROPERTY(bool passphraseError READ passphraseError)
public:
    /*!
     * \brief Constructs an EncryptedMessagePart
     * \param otp The object tree parser
     * \param text The encrypted text
     * \param protocol The cryptography protocol handler
     * \param node The MIME content node
     * \param encryptedNode The encrypted content node
     * \param parseAfterDecryption Whether to parse the content after decryption
     */
    EncryptedMessagePart(ObjectTreeParser *otp,
                         const QString &text,
                         const QGpgME::Protocol *protocol,
                         KMime::Content *node,
                         KMime::Content *encryptedNode = nullptr,
                         bool parseAfterDecryption = true);

    /*!
     * \brief Destroys the EncryptedMessagePart
     */
    ~EncryptedMessagePart() override = default;

    /*!
     * \brief Returns the encrypted message text
     * \return The encrypted text
     */
    [[nodiscard]] QString text() const override;

    /*!
     * \brief Sets whether to decrypt the message
     * \param decrypt True to decrypt the message
     */
    void setDecryptMessage(bool decrypt);
    /*!
     * \brief Returns whether the message should be decrypted
     * \return True if decryption is enabled
     */
    [[nodiscard]] bool decryptMessage() const;

    /*!
     * \brief Sets whether the message is encrypted
     * \param encrypted True if the message is encrypted
     */
    void setIsEncrypted(bool encrypted);
    /*!
     * \brief Returns whether the message is encrypted
     * \return True if the message is encrypted
     */
    [[nodiscard]] bool isEncrypted() const;

    /*!
     * \brief Returns whether the encrypted message can be decrypted
     * \return True if decryption is possible
     */
    [[nodiscard]] bool isDecryptable() const;

    /*!
     * \brief Returns whether there is no secret key to decrypt
     * \return True if no secret key is available
     */
    [[nodiscard]] bool isNoSecKey() const;
    /*!
     * \brief Returns whether there was a passphrase error
     * \return True if a passphrase error occurred
     */
    [[nodiscard]] bool passphraseError() const;

    /*!
     * \brief Starts the decryption process for the given content
     * \param data The encrypted content to decrypt
     */
    void startDecryption(KMime::Content *data);
    /*!
     * \brief Starts the decryption process
     */
    void startDecryption();

    QByteArray mDecryptedData;

    /*!
     * \brief Returns the decrypted plaintext content
     * \return The plaintext content
     */
    [[nodiscard]] QString plaintextContent() const override;
    /*!
     * \brief Returns the decrypted HTML content
     * \return The HTML content
     */
    [[nodiscard]] QString htmlContent() const override;

    /*!
     * \brief Returns the cryptography protocol handler
     * \return A pointer to the protocol handler
     */
    const QGpgME::Protocol *cryptoProto() const;

    /*!
     * \brief Returns the decryption recipients
     * \return A list of recipient/key pairs
     */
    std::vector<std::pair<GpgME::DecryptionResult::Recipient, GpgME::Key>> decryptRecipients() const;

private:
    [[nodiscard]] MIMETREEPARSER_CORE_NO_EXPORT bool decrypt(KMime::Content &data);
    bool mParseAfterDecryption{true};

protected:
    bool mPassphraseError;
    bool mNoSecKey;
    bool mDecryptMessage;
    const QGpgME::Protocol *mCryptoProto;
    QByteArray mVerifiedText;
    std::vector<std::pair<GpgME::DecryptionResult::Recipient, GpgME::Key>> mDecryptRecipients;
    KMime::Content *mEncryptedNode;
};

class MIMETREEPARSER_CORE_EXPORT SignedMessagePart : public MessagePart
{
    Q_OBJECT

public:
    /*!
     * \brief Constructs a SignedMessagePart
     * \param otp The object tree parser
     * \param protocol The cryptography protocol handler
     * \param node The MIME content node
     * \param signedData The signed content data
     * \param parseAfterDecryption Whether to parse the content after verification
     */
    SignedMessagePart(ObjectTreeParser *otp,
                      const QGpgME::Protocol *protocol,
                      KMime::Content *node,
                      KMime::Content *signedData,
                      bool parseAfterDecryption = true);

    /*!
     * \brief Destroys the SignedMessagePart
     */
    ~SignedMessagePart() override;

    /*!
     * \brief Starts the signature verification process
     */
    void startVerification();

    /*!
     * \brief Returns the plaintext content of the signed message
     * \return The plaintext representation
     */
    [[nodiscard]] QString plaintextContent() const override;
    /*!
     * \brief Returns the HTML content of the signed message
     * \return The HTML representation
     */
    [[nodiscard]] QString htmlContent() const override;

    /*!
     * \brief Returns the cryptography protocol handler
     * \return A pointer to the protocol handler
     */
    const QGpgME::Protocol *cryptoProto() const;

private:
    MIMETREEPARSER_CORE_NO_EXPORT void setVerificationResult(const GpgME::VerificationResult &result, const QByteArray &signedData);
    bool mParseAfterDecryption{true};

protected:
    const QGpgME::Protocol *mCryptoProto;
    KMime::Content *mSignedData;
    std::vector<GpgME::Signature> mSignatures;

    friend EncryptedMessagePart;
};

class MIMETREEPARSER_CORE_EXPORT HeadersPart : public MessagePart
{
    Q_OBJECT
public:
    /*!
     * \brief Constructs a HeadersPart
     * \param otp The object tree parser
     * \param node The MIME content node
     */
    HeadersPart(ObjectTreeParser *otp, KMime::Content *node);
    /*!
     * \brief Destroys the HeadersPart
     */
    ~HeadersPart() override = default;
};

}

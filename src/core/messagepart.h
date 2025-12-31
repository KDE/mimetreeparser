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
     */
    MessagePart(ObjectTreeParser *otp, const QString &text, KMime::Content *node = nullptr);

    /*!
     */
    ~MessagePart() override;

    /*!
     */
    [[nodiscard]] virtual QString text() const;
    /*!
     */
    void setText(const QString &text);
    /*!
     */
    [[nodiscard]] virtual bool isAttachment() const;

    /*!
     */
    void setIsRoot(bool root);
    /*!
     */
    [[nodiscard]] bool isRoot() const;

    /*!
     */
    void setParentPart(MessagePart *parentPart);
    /*!
     */
    [[nodiscard]] MessagePart *parentPart() const;

    /*!
     */
    [[nodiscard]] virtual QString plaintextContent() const;
    /*!
     */
    [[nodiscard]] virtual QString htmlContent() const;

    /*!
     */
    [[nodiscard]] virtual bool isHtml() const;

    /*!
     */
    [[nodiscard]] QByteArray mimeType() const;
    /*!
     */
    [[nodiscard]] QByteArray charset() const;
    /*!
     */
    [[nodiscard]] QString filename() const;
    /*!
     */
    [[nodiscard]] Disposition disposition() const;
    /*!
     */
    [[nodiscard]] bool isText() const;

    enum Error {
        NoError = 0,
        PassphraseError,
        NoKeyError,
        UnknownError,
    };

    /*!
     */
    [[nodiscard]] Error error() const;
    /*!
     */
    [[nodiscard]] QString errorString() const;

    /*!
     */
    [[nodiscard]] PartMetaData *partMetaData();

    /*!
     */
    void appendSubPart(const QSharedPointer<MessagePart> &messagePart);
    /*!
     */
    [[nodiscard]] const QList<QSharedPointer<MessagePart>> &subParts() const;
    /*!
     */
    [[nodiscard]] bool hasSubParts() const;

    /*!
     */
    KMime::Content *node() const;

    /*!
     */
    virtual KMMsgSignatureState signatureState() const;
    /*!
     */
    virtual KMMsgEncryptionState encryptionState() const;

    /*!
     */
    [[nodiscard]] QList<SignedMessagePart *> signatures() const;
    /*!
     */
    [[nodiscard]] QList<EncryptedMessagePart *> encryptions() const;

    /*!
     * Retrieve the header @header in this part or any parent parent.
     *
     * Useful for MemoryHole support.
     */
    [[nodiscard]] KMime::Headers::Base *header(const char *header) const;

    /*!
     */
    void bindLifetime(KMime::Content *);

protected:
    /*!
     */
    void parseInternal(KMime::Content *node, bool onlyOneMimePart = false);
    /*!
     */
    void parseInternal(const QByteArray &data);
    /*!
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
    MimeMessagePart(MimeTreeParser::ObjectTreeParser *otp, KMime::Content *node, bool onlyOneMimePart = false);
    ~MimeMessagePart() override;

    [[nodiscard]] QString text() const override;

    [[nodiscard]] QString plaintextContent() const override;
    [[nodiscard]] QString htmlContent() const override;

private:
    friend class AlternativeMessagePart;
};

class MIMETREEPARSER_CORE_EXPORT MessagePartList : public MessagePart
{
    Q_OBJECT
public:
    MessagePartList(MimeTreeParser::ObjectTreeParser *otp, KMime::Content *node);
    ~MessagePartList() override = default;

    [[nodiscard]] QString text() const override;

    [[nodiscard]] QString plaintextContent() const override;
    [[nodiscard]] QString htmlContent() const override;
};

class MIMETREEPARSER_CORE_EXPORT TextMessagePart : public MessagePartList
{
    Q_OBJECT
public:
    TextMessagePart(MimeTreeParser::ObjectTreeParser *otp, KMime::Content *node);
    ~TextMessagePart() override = default;

    [[nodiscard]] KMMsgSignatureState signatureState() const override;
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
    AttachmentMessagePart(MimeTreeParser::ObjectTreeParser *otp, KMime::Content *node);
    ~AttachmentMessagePart() override = default;
    [[nodiscard]] virtual bool isAttachment() const override
    {
        return true;
    }
};

class MIMETREEPARSER_CORE_EXPORT HtmlMessagePart : public MessagePart
{
    Q_OBJECT
public:
    HtmlMessagePart(MimeTreeParser::ObjectTreeParser *otp, KMime::Content *node);

    ~HtmlMessagePart() override = default;
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

    AlternativeMessagePart(MimeTreeParser::ObjectTreeParser *otp, KMime::Content *node);
    ~AlternativeMessagePart() override;

    [[nodiscard]] QString text() const override;

    [[nodiscard]] bool isHtml() const override;

    [[nodiscard]] QString plaintextContent() const override;
    [[nodiscard]] QString htmlContent() const override;
    [[nodiscard]] QString icalContent() const;

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
    CertMessagePart(MimeTreeParser::ObjectTreeParser *otp, KMime::Content *node, QGpgME::Protocol *cryptoProto);
    ~CertMessagePart() override;

    [[nodiscard]] QString text() const override;

private:
    const QGpgME::Protocol *mCryptoProto;
    GpgME::ImportResult mInportResult;
};

class MIMETREEPARSER_CORE_EXPORT EncapsulatedRfc822MessagePart : public MessagePart
{
    Q_OBJECT
public:
    EncapsulatedRfc822MessagePart(MimeTreeParser::ObjectTreeParser *otp, KMime::Content *node, const std::shared_ptr<KMime::Message> &message);
    ~EncapsulatedRfc822MessagePart() override = default;

    [[nodiscard]] QString text() const override;
    [[nodiscard]] QString from() const;
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
    EncryptedMessagePart(ObjectTreeParser *otp,
                         const QString &text,
                         const QGpgME::Protocol *protocol,
                         KMime::Content *node,
                         KMime::Content *encryptedNode = nullptr,
                         bool parseAfterDecryption = true);

    ~EncryptedMessagePart() override = default;

    [[nodiscard]] QString text() const override;

    void setDecryptMessage(bool decrypt);
    [[nodiscard]] bool decryptMessage() const;

    void setIsEncrypted(bool encrypted);
    [[nodiscard]] bool isEncrypted() const;

    [[nodiscard]] bool isDecryptable() const;

    [[nodiscard]] bool isNoSecKey() const;
    [[nodiscard]] bool passphraseError() const;

    void startDecryption(KMime::Content *data);
    void startDecryption();

    QByteArray mDecryptedData;

    [[nodiscard]] QString plaintextContent() const override;
    [[nodiscard]] QString htmlContent() const override;

    const QGpgME::Protocol *cryptoProto() const;

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
    SignedMessagePart(ObjectTreeParser *otp,
                      const QGpgME::Protocol *protocol,
                      KMime::Content *node,
                      KMime::Content *signedData,
                      bool parseAfterDecryption = true);

    ~SignedMessagePart() override;

    void startVerification();

    [[nodiscard]] QString plaintextContent() const override;
    [[nodiscard]] QString htmlContent() const override;

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
    HeadersPart(ObjectTreeParser *otp, KMime::Content *node);
    ~HeadersPart() override = default;
};

}

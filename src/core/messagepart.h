// SPDX-FileCopyrightText: 2015 Sandro Knauß <knauss@kolabsys.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

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

/** Flags for the encryption state. */
typedef enum { KMMsgEncryptionStateUnknown, KMMsgNotEncrypted, KMMsgPartiallyEncrypted, KMMsgFullyEncrypted, KMMsgEncryptionProblematic } KMMsgEncryptionState;

/** Flags for the signature state. */
typedef enum { KMMsgSignatureStateUnknown, KMMsgNotSigned, KMMsgPartiallySigned, KMMsgFullySigned, KMMsgSignatureProblematic } KMMsgSignatureState;

class ObjectTreeParser;
class MultiPartAlternativeBodyPartFormatter;

class SignedMessagePart;
class EncryptedMessagePart;

class MIMETREEPARSER_CORE_EXPORT MessagePart : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool attachment READ isAttachment CONSTANT)
    Q_PROPERTY(bool root READ isRoot CONSTANT)
    Q_PROPERTY(bool isHtml READ isHtml CONSTANT)
    Q_PROPERTY(QString plaintextContent READ plaintextContent CONSTANT)
    Q_PROPERTY(QString htmlContent READ htmlContent CONSTANT)
public:
    enum Disposition { Inline, Attachment, Invalid };
    using Ptr = QSharedPointer<MessagePart>;
    using List = QVector<Ptr>;
    MessagePart(ObjectTreeParser *otp, const QString &text, KMime::Content *node = nullptr);

    virtual ~MessagePart();

    virtual QString text() const;
    void setText(const QString &text);
    virtual bool isAttachment() const;

    void setIsRoot(bool root);
    bool isRoot() const;

    void setParentPart(MessagePart *parentPart);
    MessagePart *parentPart() const;

    virtual QString plaintextContent() const;
    virtual QString htmlContent() const;

    virtual bool isHtml() const;

    QByteArray mimeType() const;
    QByteArray charset() const;
    QString filename() const;
    Disposition disposition() const;
    bool isText() const;

    enum Error { NoError = 0, PassphraseError, NoKeyError, UnknownError };

    Error error() const;
    QString errorString() const;

    PartMetaData *partMetaData();

    void appendSubPart(const MessagePart::Ptr &messagePart);
    const QVector<MessagePart::Ptr> &subParts() const;
    bool hasSubParts() const;

    KMime::Content *node() const;

    virtual KMMsgSignatureState signatureState() const;
    virtual KMMsgEncryptionState encryptionState() const;

    QVector<SignedMessagePart *> signatures() const;
    QVector<EncryptedMessagePart *> encryptions() const;

    /**
     * Retrieve the header @header in this part or any parent parent.
     *
     * Useful for MemoryHole support.
     */
    KMime::Headers::Base *header(const char *header) const;

    void bindLifetime(KMime::Content *);

protected:
    void parseInternal(KMime::Content *node, bool onlyOneMimePart = false);
    void parseInternal(const QByteArray &data);
    QString renderInternalText() const;

    QString mText;
    ObjectTreeParser *mOtp;
    PartMetaData mMetaData;
    MessagePart *mParentPart;
    KMime::Content *mNode;
    QVector<KMime::Content *> mNodesToDelete;
    Error mError;

private:
    QVector<MessagePart::Ptr> mBlocks;
    bool mRoot;
};

class MIMETREEPARSER_CORE_EXPORT MimeMessagePart : public MessagePart
{
    Q_OBJECT
public:
    typedef QSharedPointer<MimeMessagePart> Ptr;
    MimeMessagePart(MimeTreeParser::ObjectTreeParser *otp, KMime::Content *node, bool onlyOneMimePart = false);
    virtual ~MimeMessagePart();

    QString text() const Q_DECL_OVERRIDE;

    QString plaintextContent() const Q_DECL_OVERRIDE;
    QString htmlContent() const Q_DECL_OVERRIDE;

private:
    friend class AlternativeMessagePart;
};

class MIMETREEPARSER_CORE_EXPORT MessagePartList : public MessagePart
{
    Q_OBJECT
public:
    typedef QSharedPointer<MessagePartList> Ptr;
    MessagePartList(MimeTreeParser::ObjectTreeParser *otp, KMime::Content *node);
    virtual ~MessagePartList() = default;

    QString text() const Q_DECL_OVERRIDE;

    QString plaintextContent() const Q_DECL_OVERRIDE;
    QString htmlContent() const Q_DECL_OVERRIDE;
};

class MIMETREEPARSER_CORE_EXPORT TextMessagePart : public MessagePartList
{
    Q_OBJECT
public:
    typedef QSharedPointer<TextMessagePart> Ptr;
    TextMessagePart(MimeTreeParser::ObjectTreeParser *otp, KMime::Content *node);
    virtual ~TextMessagePart() = default;

    KMMsgSignatureState signatureState() const Q_DECL_OVERRIDE;
    KMMsgEncryptionState encryptionState() const Q_DECL_OVERRIDE;

private:
    void parseContent();

    KMMsgSignatureState mSignatureState;
    KMMsgEncryptionState mEncryptionState;

    friend class ObjectTreeParser;
};

class MIMETREEPARSER_CORE_EXPORT AttachmentMessagePart : public TextMessagePart
{
    Q_OBJECT
public:
    typedef QSharedPointer<AttachmentMessagePart> Ptr;
    AttachmentMessagePart(MimeTreeParser::ObjectTreeParser *otp, KMime::Content *node);
    virtual ~AttachmentMessagePart() = default;
    virtual bool isAttachment() const Q_DECL_OVERRIDE
    {
        return true;
    }
};

class MIMETREEPARSER_CORE_EXPORT HtmlMessagePart : public MessagePart
{
    Q_OBJECT
public:
    typedef QSharedPointer<HtmlMessagePart> Ptr;
    HtmlMessagePart(MimeTreeParser::ObjectTreeParser *otp, KMime::Content *node);
    virtual ~HtmlMessagePart() = default;
    bool isHtml() const Q_DECL_OVERRIDE
    {
        return true;
    };
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

    typedef QSharedPointer<AlternativeMessagePart> Ptr;
    AlternativeMessagePart(MimeTreeParser::ObjectTreeParser *otp, KMime::Content *node);
    virtual ~AlternativeMessagePart();

    QString text() const Q_DECL_OVERRIDE;

    bool isHtml() const Q_DECL_OVERRIDE;

    QString plaintextContent() const Q_DECL_OVERRIDE;
    QString htmlContent() const Q_DECL_OVERRIDE;
    QString icalContent() const;

    QList<HtmlMode> availableModes();

private:
    QMap<HtmlMode, MessagePart::Ptr> mChildParts;

    friend class ObjectTreeParser;
    friend class MultiPartAlternativeBodyPartFormatter;
};

class MIMETREEPARSER_CORE_EXPORT CertMessagePart : public MessagePart
{
    Q_OBJECT
public:
    typedef QSharedPointer<CertMessagePart> Ptr;
    CertMessagePart(MimeTreeParser::ObjectTreeParser *otp, KMime::Content *node, QGpgME::Protocol *cryptoProto);
    virtual ~CertMessagePart();

    QString text() const Q_DECL_OVERRIDE;

private:
    const QGpgME::Protocol *mCryptoProto;
    GpgME::ImportResult mInportResult;
};

class MIMETREEPARSER_CORE_EXPORT EncapsulatedRfc822MessagePart : public MessagePart
{
    Q_OBJECT
public:
    typedef QSharedPointer<EncapsulatedRfc822MessagePart> Ptr;
    EncapsulatedRfc822MessagePart(MimeTreeParser::ObjectTreeParser *otp, KMime::Content *node, const KMime::Message::Ptr &message);
    virtual ~EncapsulatedRfc822MessagePart() = default;

    QString text() const Q_DECL_OVERRIDE;
    QString from() const;
    QDateTime date() const;

private:
    const KMime::Message::Ptr mMessage;
};

class MIMETREEPARSER_CORE_EXPORT EncryptedMessagePart : public MessagePart
{
    Q_OBJECT
    Q_PROPERTY(bool decryptMessage READ decryptMessage WRITE setDecryptMessage)
    Q_PROPERTY(bool isEncrypted READ isEncrypted)
    Q_PROPERTY(bool isNoSecKey READ isNoSecKey)
    Q_PROPERTY(bool passphraseError READ passphraseError)
public:
    typedef QSharedPointer<EncryptedMessagePart> Ptr;
    EncryptedMessagePart(ObjectTreeParser *otp,
                         const QString &text,
                         const QGpgME::Protocol *protocol,
                         KMime::Content *node,
                         KMime::Content *encryptedNode = nullptr,
                         bool parseAfterDecryption = true);

    virtual ~EncryptedMessagePart() = default;

    QString text() const Q_DECL_OVERRIDE;

    void setDecryptMessage(bool decrypt);
    Q_REQUIRED_RESULT bool decryptMessage() const;

    void setIsEncrypted(bool encrypted);
    Q_REQUIRED_RESULT bool isEncrypted() const;

    Q_REQUIRED_RESULT bool isDecryptable() const;

    Q_REQUIRED_RESULT bool isNoSecKey() const;
    Q_REQUIRED_RESULT bool passphraseError() const;

    void startDecryption(KMime::Content *data);
    void startDecryption();

    QByteArray mDecryptedData;

    QString plaintextContent() const override;
    QString htmlContent() const override;

    const QGpgME::Protocol *cryptoProto() const;

    std::vector<std::pair<GpgME::DecryptionResult::Recipient, GpgME::Key>> decryptRecipients() const;

private:
    bool decrypt(KMime::Content &data);
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
    Q_PROPERTY(bool isSigned READ isSigned CONSTANT)
public:
    typedef QSharedPointer<SignedMessagePart> Ptr;
    SignedMessagePart(ObjectTreeParser *otp,
                      const QGpgME::Protocol *protocol,
                      KMime::Content *node,
                      KMime::Content *signedData,
                      bool parseAfterDecryption = true);

    virtual ~SignedMessagePart();

    void setIsSigned(bool isSigned);
    bool isSigned() const;

    void startVerification();

    QString plaintextContent() const Q_DECL_OVERRIDE;
    QString htmlContent() const Q_DECL_OVERRIDE;

    const QGpgME::Protocol *cryptoProto() const;

private:
    void sigStatusToMetaData();
    void setVerificationResult(const GpgME::VerificationResult &result, const QByteArray &signedData);
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
    typedef QSharedPointer<HeadersPart> Ptr;
    HeadersPart(ObjectTreeParser *otp, KMime::Content *node);
    virtual ~HeadersPart() = default;
};

}

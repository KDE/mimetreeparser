// SPDX-FileCopyrightText: 2015 Sandro Knauß <knauss@kolabsys.com>
// SPDX-FileCopyrightText: 2017 Christian Mollekopf <mollekopf@kolabsys.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "messagepart.h"
#include "cryptohelper.h"
#include "mimetreeparser_core_debug.h"
#include "objecttreeparser.h"

#include "utils.h"

#include <KLocalizedString>
#include <KMime/Content>
#include <Libkleo/Compliance>
#include <Libkleo/Formatting>
#include <Libkleo/KeyCache>

#include <QGpgME/DN>
#include <QGpgME/DecryptVerifyJob>
#include <QGpgME/Protocol>
#include <QGpgME/VerifyDetachedJob>
#include <QGpgME/VerifyOpaqueJob>

#include <QStringDecoder>

#include <gpgme++/key.h>
#include <gpgme++/keylistresult.h>
#include <gpgme.h>
using namespace Qt::Literals::StringLiterals;

using namespace MimeTreeParser;

//------MessagePart-----------------------
MessagePart::MessagePart(ObjectTreeParser *otp, const QString &text, KMime::Content *node)
    : mText(text)
    , mOtp(otp)
    , mParentPart(nullptr)
    , mNode(node) // only null for messagepartlist
    , mError(NoError)
    , mRoot(false)
{
}

MessagePart::~MessagePart()
{
    for (auto n : std::as_const(mNodesToDelete)) {
        delete n;
    }
}

MessagePart::Disposition MessagePart::disposition() const
{
    if (!mNode) {
        return Invalid;
    }
    const auto cd = mNode->contentDisposition(false);
    if (!cd) {
        return Invalid;
    }
    switch (cd->disposition()) {
    case KMime::Headers::CDinline:
        return Inline;
    case KMime::Headers::CDattachment:
        return Attachment;
    default:
        return Invalid;
    }
}

QString MessagePart::filename() const
{
    if (!mNode) {
        return {};
    }

    if (const auto cd = mNode->contentDisposition(false)) {
        const auto name = cd->filename();
        // Allow for a fallback for mails that have a ContentDisposition header, but don't set the filename anyways.
        // Not the recommended way, but exists.
        if (!name.isEmpty()) {
            return name;
        }
    }
    if (const auto ct = mNode->contentType(false)) {
        return ct->name();
    }
    return {};
}

static KMime::Headers::ContentType *contentType(KMime::Content *node)
{
    if (node) {
        return node->contentType(false);
    }
    return nullptr;
}

QByteArray MessagePart::charset() const
{
    if (!mNode) {
        return "us-ascii"_ba;
    }
    if (auto ct = contentType(mNode)) {
        return ct->charset();
    }
    // Per rfc2045 us-ascii is the default
    return "us-ascii"_ba;
}

QByteArray MessagePart::mimeType() const
{
    if (auto ct = contentType(mNode)) {
        return ct->mimeType();
    }
    return {};
}

bool MessagePart::isText() const
{
    if (auto ct = contentType(mNode)) {
        return ct->isText();
    }
    return false;
}

MessagePart::Error MessagePart::error() const
{
    return mError;
}

QString MessagePart::errorString() const
{
    return mMetaData.errorText;
}

PartMetaData *MessagePart::partMetaData()
{
    return &mMetaData;
}

bool MessagePart::isAttachment() const
{
    if (mNode) {
        return KMime::isAttachment(mNode);
    }
    return false;
}

KMime::Content *MessagePart::node() const
{
    return mNode;
}

void MessagePart::setIsRoot(bool root)
{
    mRoot = root;
}

bool MessagePart::isRoot() const
{
    return mRoot;
}

QString MessagePart::text() const
{
    return mText;
}

void MessagePart::setText(const QString &text)
{
    mText = text;
}

bool MessagePart::isHtml() const
{
    return false;
}

MessagePart *MessagePart::parentPart() const
{
    return mParentPart;
}

void MessagePart::setParentPart(MessagePart *parentPart)
{
    mParentPart = parentPart;
}

QString MessagePart::htmlContent() const
{
    return text();
}

QString MessagePart::plaintextContent() const
{
    return text();
}

void MessagePart::parseInternal(KMime::Content *node, bool onlyOneMimePart)
{
    auto subMessagePart = mOtp->parseObjectTreeInternal(node, onlyOneMimePart);
    mRoot = subMessagePart->isRoot();
    for (const auto &part : std::as_const(subMessagePart->subParts())) {
        appendSubPart(part);
    }
}

void MessagePart::parseInternal(const QByteArray &data)
{
    auto tempNode = new KMime::Content();

    const auto lfData = KMime::CRLFtoLF(data);
    // We have to deal with both bodies and full parts. In inline encrypted/signed parts we can have nested parts,
    // or just plain-text, and both ends up here. setContent defaults to setting only the header, so we have to avoid this.
    if (lfData.contains("\n\n")) {
        tempNode->setContent(lfData);
    } else {
        tempNode->setBody(lfData);
    }
    tempNode->parse();
    tempNode->contentType()->setCharset(charset());
    bindLifetime(tempNode);

    if (!tempNode->head().isEmpty()) {
        tempNode->contentDescription()->from7BitString("temporary node"_ba);
    }

    parseInternal(tempNode);
}

QString MessagePart::renderInternalText() const
{
    QString text;
    for (const auto &mp : subParts()) {
        text += mp->text();
    }
    return text;
}

void MessagePart::appendSubPart(const MessagePart::Ptr &messagePart)
{
    messagePart->setParentPart(this);
    mBlocks.append(messagePart);
}

const QList<MessagePart::Ptr> &MessagePart::subParts() const
{
    return mBlocks;
}

bool MessagePart::hasSubParts() const
{
    return !mBlocks.isEmpty();
}

QList<SignedMessagePart *> MessagePart::signatures() const
{
    QList<SignedMessagePart *> list;
    if (auto sig = dynamic_cast<SignedMessagePart *>(const_cast<MessagePart *>(this))) {
        list << sig;
    }
    auto parent = parentPart();
    while (parent) {
        if (auto sig = dynamic_cast<SignedMessagePart *>(parent)) {
            list << sig;
        }
        parent = parent->parentPart();
    }
    return list;
}

QList<EncryptedMessagePart *> MessagePart::encryptions() const
{
    QList<EncryptedMessagePart *> list;
    if (auto sig = dynamic_cast<EncryptedMessagePart *>(const_cast<MessagePart *>(this))) {
        list << sig;
    }
    auto parent = parentPart();
    while (parent) {
        if (auto sig = dynamic_cast<EncryptedMessagePart *>(parent)) {
            list << sig;
        }
        parent = parent->parentPart();
    }
    return list;
}

KMMsgEncryptionState MessagePart::encryptionState() const
{
    if (!encryptions().isEmpty()) {
        return KMMsgFullyEncrypted;
    }
    return KMMsgNotEncrypted;
}

KMMsgSignatureState MessagePart::signatureState() const
{
    if (!signatures().isEmpty()) {
        return KMMsgFullySigned;
    }
    return KMMsgNotSigned;
}

void MessagePart::bindLifetime(KMime::Content *node)
{
    mNodesToDelete << node;
}

KMime::Headers::Base *MimeTreeParser::MessagePart::header(const char *header) const
{
    if (node() && node()->hasHeader(header)) {
        return node()->headerByType(header);
    }
    if (auto parent = parentPart()) {
        return parent->header(header);
    }

    return nullptr;
}

//-----MessagePartList----------------------
MessagePartList::MessagePartList(ObjectTreeParser *otp, KMime::Content *node)
    : MessagePart(otp, QString(), node)
{
}

QString MessagePartList::text() const
{
    return renderInternalText();
}

QString MessagePartList::plaintextContent() const
{
    return QString();
}

QString MessagePartList::htmlContent() const
{
    return QString();
}

//-----TextMessageBlock----------------------

TextMessagePart::TextMessagePart(ObjectTreeParser *otp, KMime::Content *node)
    : MessagePartList(otp, node)
    , mSignatureState(KMMsgSignatureStateUnknown)
    , mEncryptionState(KMMsgEncryptionStateUnknown)
{
    if (!mNode) {
        qCWarning(MIMETREEPARSER_CORE_LOG) << "not a valid node";
        return;
    }

    parseContent();
}

void TextMessagePart::parseContent()
{
    mSignatureState = KMMsgNotSigned;
    mEncryptionState = KMMsgNotEncrypted;
    const auto blocks = prepareMessageForDecryption(mNode->decodedContent());
    // We also get blocks for unencrypted messages
    if (!blocks.isEmpty()) {
        auto aCodec = QStringDecoder(mOtp->codecNameFor(mNode).constData());
        const auto cryptProto = QGpgME::openpgp();

        /* The (overall) signature/encrypted status is broken
         * if one unencrypted part is at the beginning or in the middle
         * because mailmain adds an unencrypted part at the end this should not break the overall status
         *
         * That's why we first set the tmp status and if one crypted/signed block comes afterwards, than
         * the status is set to unencryped
         */
        bool fullySignedOrEncrypted = true;
        bool fullySignedOrEncryptedTmp = true;

        for (const auto &block : blocks) {
            if (!fullySignedOrEncryptedTmp) {
                fullySignedOrEncrypted = false;
            }

            if (block.type() == NoPgpBlock && !block.text().trimmed().isEmpty()) {
                fullySignedOrEncryptedTmp = false;
                appendSubPart(MessagePart::Ptr(new MessagePart(mOtp, aCodec.decode(KMime::CRLFtoLF(block.text())))));
            } else if (block.type() == PgpMessageBlock) {
                auto content = new KMime::Content;
                content->setBody(block.text());
                content->parse();
                content->contentType()->setCharset(charset());
                EncryptedMessagePart::Ptr mp(new EncryptedMessagePart(mOtp, QString(), cryptProto, content, content, false));
                mp->bindLifetime(content);
                mp->setIsEncrypted(true);
                appendSubPart(mp);
            } else if (block.type() == ClearsignedBlock) {
                auto content = new KMime::Content;
                content->setBody(block.text());
                content->parse();
                content->contentType()->setCharset(charset());
                SignedMessagePart::Ptr mp(new SignedMessagePart(mOtp, cryptProto, nullptr, content, false));
                mp->bindLifetime(content);
                appendSubPart(mp);
            } else {
                continue;
            }

            const auto mp = subParts().last().staticCast<MessagePart>();
            const PartMetaData *messagePart(mp->partMetaData());

            if (!messagePart->isEncrypted && !messagePart->isSigned() && !block.text().trimmed().isEmpty()) {
                mp->setText(aCodec.decode(KMime::CRLFtoLF(block.text())));
            }

            if (messagePart->isEncrypted) {
                mEncryptionState = KMMsgPartiallyEncrypted;
            }

            if (messagePart->isSigned()) {
                mSignatureState = KMMsgPartiallySigned;
            }
        }

        // Do we have an fully Signed/Encrypted Message?
        if (fullySignedOrEncrypted) {
            if (mSignatureState == KMMsgPartiallySigned) {
                mSignatureState = KMMsgFullySigned;
            }
            if (mEncryptionState == KMMsgPartiallyEncrypted) {
                mEncryptionState = KMMsgFullyEncrypted;
            }
        }
    }
}

KMMsgEncryptionState TextMessagePart::encryptionState() const
{
    if (mEncryptionState == KMMsgNotEncrypted) {
        return MessagePart::encryptionState();
    }
    return mEncryptionState;
}

KMMsgSignatureState TextMessagePart::signatureState() const
{
    if (mSignatureState == KMMsgNotSigned) {
        return MessagePart::signatureState();
    }
    return mSignatureState;
}

//-----AttachmentMessageBlock----------------------

AttachmentMessagePart::AttachmentMessagePart(ObjectTreeParser *otp, KMime::Content *node)
    : TextMessagePart(otp, node)
{
}

//-----HtmlMessageBlock----------------------

HtmlMessagePart::HtmlMessagePart(ObjectTreeParser *otp, KMime::Content *node)
    : MessagePart(otp, QString(), node)
{
    if (!mNode) {
        qCWarning(MIMETREEPARSER_CORE_LOG) << "not a valid node";
        return;
    }

    setText(QStringDecoder(mOtp->codecNameFor(mNode).constData()).decode(KMime::CRLFtoLF(mNode->decodedContent())));
}

//-----MimeMessageBlock----------------------

MimeMessagePart::MimeMessagePart(ObjectTreeParser *otp, KMime::Content *node, bool onlyOneMimePart)
    : MessagePart(otp, QString(), node)
{
    if (!mNode) {
        qCWarning(MIMETREEPARSER_CORE_LOG) << "not a valid node";
        return;
    }

    parseInternal(mNode, onlyOneMimePart);
}

MimeMessagePart::~MimeMessagePart()
{
}

QString MimeMessagePart::text() const
{
    return renderInternalText();
}

QString MimeMessagePart::plaintextContent() const
{
    return QString();
}

QString MimeMessagePart::htmlContent() const
{
    return QString();
}

//-----AlternativeMessagePart----------------------

AlternativeMessagePart::AlternativeMessagePart(ObjectTreeParser *otp, KMime::Content *node)
    : MessagePart(otp, QString(), node)
{
    if (auto dataIcal = findTypeInDirectChildren(mNode, "text/calendar")) {
        mChildParts[MultipartIcal] = MimeMessagePart::Ptr(new MimeMessagePart(mOtp, dataIcal, true));
    }

    if (auto dataText = findTypeInDirectChildren(mNode, "text/plain")) {
        mChildParts[MultipartPlain] = MimeMessagePart::Ptr(new MimeMessagePart(mOtp, dataText, true));
    }

    if (auto dataHtml = findTypeInDirectChildren(mNode, "text/html")) {
        mChildParts[MultipartHtml] = MimeMessagePart::Ptr(new MimeMessagePart(mOtp, dataHtml, true));
    } else {
        // If we didn't find the HTML part as the first child of the multipart/alternative, it might
        // be that this is a HTML message with images, and text/plain and multipart/related are the
        // immediate children of this multipart/alternative node.
        // In this case, the HTML node is a child of multipart/related.
        // In the case of multipart/related we don't expect multiple html parts, it is usually used to group attachments
        // with html content.
        //
        // In any case, this is not a complete implementation of MIME, but an approximation for the kind of mails we actually see in the wild.
        auto data = [&] {
            if (auto d = findTypeInDirectChildren(mNode, "multipart/related")) {
                return d;
            }
            return findTypeInDirectChildren(mNode, "multipart/mixed");
        }();
        if (data) {
            QString htmlContent;
            const auto parts = data->contents();
            for (auto p : parts) {
                if ((!p->contentType()->isEmpty()) && (p->contentType()->mimeType() == "text/html")) {
                    htmlContent += MimeMessagePart(mOtp, p, true).text();
                } else if (KMime::isAttachment(p)) {
                    appendSubPart(MimeMessagePart::Ptr(new MimeMessagePart(otp, p, true)));
                }
            }
            mChildParts[MultipartHtml] = MessagePart::Ptr(new MessagePart(mOtp, htmlContent, nullptr));
        }
    }
}

AlternativeMessagePart::~AlternativeMessagePart()
{
}

QList<AlternativeMessagePart::HtmlMode> AlternativeMessagePart::availableModes()
{
    return mChildParts.keys();
}

QString AlternativeMessagePart::text() const
{
    if (mChildParts.contains(MultipartPlain)) {
        return mChildParts[MultipartPlain]->text();
    }
    return QString();
}

bool AlternativeMessagePart::isHtml() const
{
    return mChildParts.contains(MultipartHtml);
}

QString AlternativeMessagePart::plaintextContent() const
{
    return text();
}

QString AlternativeMessagePart::htmlContent() const
{
    if (mChildParts.contains(MultipartHtml)) {
        return mChildParts[MultipartHtml]->text();
    } else {
        return plaintextContent();
    }
}

QString AlternativeMessagePart::icalContent() const
{
    if (mChildParts.contains(MultipartIcal)) {
        return mChildParts[MultipartIcal]->text();
    }
    return {};
}

//-----CertMessageBlock----------------------

CertMessagePart::CertMessagePart(ObjectTreeParser *otp, KMime::Content *node, QGpgME::Protocol *cryptoProto)
    : MessagePart(otp, QString(), node)
    , mCryptoProto(cryptoProto)
{
    if (!mNode) {
        qCWarning(MIMETREEPARSER_CORE_LOG) << "not a valid node";
        return;
    }
}

CertMessagePart::~CertMessagePart()
{
}

QString CertMessagePart::text() const
{
    return QString();
}

//-----SignedMessageBlock---------------------
SignedMessagePart::SignedMessagePart(ObjectTreeParser *otp,
                                     const QGpgME::Protocol *cryptoProto,
                                     KMime::Content *node,
                                     KMime::Content *signedData,
                                     bool parseAfterDecryption)
    : MessagePart(otp, {}, node)
    , mParseAfterDecryption(parseAfterDecryption)
    , mCryptoProto(cryptoProto)
    , mSignedData(signedData)
{
    mMetaData.status = i18ndc("mimetreeparser", "@info:status", "Wrong Crypto Plug-In.");
}

SignedMessagePart::~SignedMessagePart() = default;

const QGpgME::Protocol *SignedMessagePart::cryptoProto() const
{
    return mCryptoProto;
}

void SignedMessagePart::startVerification()
{
    if (!mSignedData) {
        return;
    }

    mMetaData.status = i18ndc("mimetreeparser", "@info:status", "Wrong Crypto Plug-In.");
    mMetaData.isEncrypted = false;
    mMetaData.isDecryptable = false;

    auto codec = QStringDecoder(mOtp->codecNameFor(mSignedData).constData());

    // If we have a mNode, this is a detached signature
    if (mNode) {
        const auto signature = mNode->decodedContent();

        // This is necessary in case the original data contained CRLF's. Otherwise the signature will not match the data (since KMIME normalizes to LF)
        const QByteArray signedData = KMime::LFtoCRLF(mSignedData->encodedContent());

        const auto job = mCryptoProto->verifyDetachedJob();
        setVerificationResult(job->exec(signature, signedData), signedData);
        job->deleteLater();
        setText(codec.decode(KMime::CRLFtoLF(signedData)));
    } else {
        QByteArray outdata;
        const auto job = mCryptoProto->verifyOpaqueJob();
        setVerificationResult(job->exec(mSignedData->decodedContent(), outdata), outdata);
        job->deleteLater();
        setText(codec.decode(KMime::CRLFtoLF(outdata)));
    }
}

void SignedMessagePart::setVerificationResult(const GpgME::VerificationResult &result, const QByteArray &signedData)
{
    mMetaData.verificationResult = result;

    if (mMetaData.isSigned()) {
        if (!signedData.isEmpty() && mParseAfterDecryption) {
            parseInternal(signedData);
        }
    }
}

QString SignedMessagePart::plaintextContent() const
{
    if (!mNode) {
        return MessagePart::text();
    } else {
        return QString();
    }
}

QString SignedMessagePart::htmlContent() const
{
    if (!mNode) {
        return MessagePart::text();
    } else {
        return QString();
    }
}

//-----CryptMessageBlock---------------------
EncryptedMessagePart::EncryptedMessagePart(ObjectTreeParser *otp,
                                           const QString &text,
                                           const QGpgME::Protocol *cryptoProto,
                                           KMime::Content *node,
                                           KMime::Content *encryptedNode,
                                           bool parseAfterDecryption)
    : MessagePart(otp, text, node)
    , mParseAfterDecryption(parseAfterDecryption)
    , mPassphraseError(false)
    , mNoSecKey(false)
    , mDecryptMessage(false)
    , mCryptoProto(cryptoProto)
    , mEncryptedNode(encryptedNode)
{
    mMetaData.isEncrypted = false;
    mMetaData.isDecryptable = false;
    mMetaData.status = i18ndc("mimetreeparser", "@info:status", "Wrong Crypto Plug-In.");
}

void EncryptedMessagePart::setIsEncrypted(bool encrypted)
{
    mMetaData.isEncrypted = encrypted;
}

bool EncryptedMessagePart::isEncrypted() const
{
    return mMetaData.isEncrypted;
}

const QGpgME::Protocol *EncryptedMessagePart::cryptoProto() const
{
    return mCryptoProto;
}

void EncryptedMessagePart::setDecryptMessage(bool decrypt)
{
    mDecryptMessage = decrypt;
}

bool EncryptedMessagePart::decryptMessage() const
{
    return mDecryptMessage;
}

bool EncryptedMessagePart::isDecryptable() const
{
    return mMetaData.isDecryptable;
}

bool EncryptedMessagePart::isNoSecKey() const
{
    return mNoSecKey;
}

bool EncryptedMessagePart::passphraseError() const
{
    return mPassphraseError;
}

bool EncryptedMessagePart::decrypt(KMime::Content &data)
{
    mError = NoError;
    mMetaData.errorText.clear();
    // FIXME
    //  mMetaData.auditLogError = GpgME::Error();
    mMetaData.auditLog.clear();

    const QByteArray ciphertext = data.decodedContent();
    QByteArray plainText;
    auto job = mCryptoProto->decryptVerifyJob();
    const std::pair<GpgME::DecryptionResult, GpgME::VerificationResult> p = job->exec(ciphertext, plainText);
    job->deleteLater();
    auto decryptResult = p.first;
    auto verifyResult = p.second;
    mMetaData.verificationResult = verifyResult;

    // Normalize CRLF's
    plainText = KMime::CRLFtoLF(plainText);
    auto codec = QStringDecoder(mOtp->codecNameFor(&data).constData());
    const auto decoded = codec.decode(plainText);

    if (partMetaData()->isSigned()) {
        // We simply attach a signed message part to indicate that this content is also signed
        // We're forwarding mNode to not loose the encoding information
        auto subPart = SignedMessagePart::Ptr(new SignedMessagePart(mOtp, mCryptoProto, mNode, nullptr));
        subPart->setText(decoded);
        subPart->setVerificationResult(verifyResult, plainText);
        appendSubPart(subPart);
    }

    mDecryptRecipients.clear();
    bool cannotDecrypt = false;
    bool bDecryptionOk = !decryptResult.error();

    for (const auto &recipient : decryptResult.recipients()) {
        if (!recipient.status()) {
            bDecryptionOk = true;
        }
        GpgME::Key key;
        key = Kleo::KeyCache::instance()->findByKeyIDOrFingerprint(recipient.keyID());
        if (key.isNull()) {
            auto ret = Kleo::KeyCache::instance()->findSubkeysByKeyID({recipient.keyID()});
            if (ret.size() == 1) {
                key = ret.front().parent();
            }
            if (key.isNull()) {
                qCDebug(MIMETREEPARSER_CORE_LOG) << "Found no Key for KeyID " << recipient.keyID();
            }
        }
        mDecryptRecipients.emplace_back(recipient, key);
    }

    if (!bDecryptionOk && partMetaData()->isSigned()) {
        // Only a signed part
        partMetaData()->isEncrypted = false;
        bDecryptionOk = true;
        mDecryptedData = plainText;
    } else {
        mPassphraseError = decryptResult.error().isCanceled() || decryptResult.error().code() == GPG_ERR_BAD_PASSPHRASE;
        mMetaData.isEncrypted = bDecryptionOk || decryptResult.error().code() != GPG_ERR_NO_DATA;

        if (decryptResult.error().isCanceled()) {
            setDecryptMessage(false);
        }

        partMetaData()->errorText = Kleo::Formatting::errorAsString(decryptResult.error());
        if (Kleo::DeVSCompliance::isCompliant()) {
            partMetaData()->isCompliant = decryptResult.isDeVs();
            partMetaData()->compliance = Kleo::DeVSCompliance::name(decryptResult.isDeVs());
        } else {
            partMetaData()->isCompliant = true;
        }
        if (partMetaData()->isEncrypted && decryptResult.numRecipients() > 0) {
            partMetaData()->keyId = decryptResult.recipient(0).keyID();
        }

        if (bDecryptionOk) {
            mDecryptedData = plainText;
        } else {
            mNoSecKey = true;
            const auto decryRecipients = decryptResult.recipients();
            for (const GpgME::DecryptionResult::Recipient &recipient : decryRecipients) {
                mNoSecKey &= (recipient.status().code() == GPG_ERR_NO_SECKEY);
            }
            if (!mPassphraseError && !mNoSecKey) { // GpgME do not detect passphrase error correctly
                mPassphraseError = true;
            }
        }
    }

    if (!bDecryptionOk) {
        QString cryptPlugLibName;
        mError = UnknownError;
        if (mCryptoProto) {
            cryptPlugLibName = mCryptoProto->name();
        }

        if (mNoSecKey) {
            mError = NoKeyError;
        }

        if (mPassphraseError) {
            mError = PassphraseError;
        }

        if (!mCryptoProto) {
            partMetaData()->errorText = i18n("No appropriate crypto plug-in was found.");
        } else if (cannotDecrypt) {
            partMetaData()->errorText = i18n("Crypto plug-in \"%1\" cannot decrypt messages.", cryptPlugLibName);
        } else if (!passphraseError()) {
            partMetaData()->errorText = i18n("Crypto plug-in \"%1\" could not decrypt the data.", cryptPlugLibName) + QLatin1StringView("<br />")
                + i18n("Error: %1", partMetaData()->errorText);
        }
    }
    return bDecryptionOk;
}

void EncryptedMessagePart::startDecryption(KMime::Content *data)
{
    mMetaData.isEncrypted = true;
    mMetaData.isDecryptable = decrypt(*data);

    if (mParseAfterDecryption && !mMetaData.isSigned()) {
        parseInternal(mDecryptedData);
    } else {
        setText(QString::fromUtf8(mDecryptedData.constData()));
    }
}

void EncryptedMessagePart::startDecryption()
{
    if (mEncryptedNode) {
        startDecryption(mEncryptedNode);
    } else {
        startDecryption(mNode);
    }
}

std::vector<std::pair<GpgME::DecryptionResult::Recipient, GpgME::Key>> EncryptedMessagePart::decryptRecipients() const
{
    return mDecryptRecipients;
}

QString EncryptedMessagePart::plaintextContent() const
{
    if (!mNode) {
        return MessagePart::text();
    } else {
        return QString();
    }
}

QString EncryptedMessagePart::htmlContent() const
{
    if (!mNode) {
        return MessagePart::text();
    } else {
        return QString();
    }
}

QString EncryptedMessagePart::text() const
{
    if (hasSubParts()) {
        auto _mp = (subParts()[0]).dynamicCast<SignedMessagePart>();
        if (_mp) {
            return _mp->text();
        }
    }

    return MessagePart::text();
}

EncapsulatedRfc822MessagePart::EncapsulatedRfc822MessagePart(ObjectTreeParser *otp, KMime::Content *node, const KMime::Message::Ptr &message)
    : MessagePart(otp, QString(), node)
    , mMessage(message)
{
    mMetaData.isEncrypted = false;
    mMetaData.isEncapsulatedRfc822Message = true;

    if (!mMessage) {
        qCWarning(MIMETREEPARSER_CORE_LOG) << "Node is of type message/rfc822 but doesn't have a message!";
        return;
    }

    parseInternal(message.data());
}

QString EncapsulatedRfc822MessagePart::text() const
{
    return renderInternalText();
}

QString EncapsulatedRfc822MessagePart::from() const
{
    if (auto from = mMessage->from(false)) {
        return from->asUnicodeString();
    }
    return {};
}

QDateTime EncapsulatedRfc822MessagePart::date() const
{
    if (auto date = mMessage->date(false)) {
        return date->dateTime();
    }
    return {};
}

HeadersPart::HeadersPart(ObjectTreeParser *otp, KMime::Content *node)
    : MessagePart(otp, QString(), node)
{
}

#include "moc_messagepart.cpp"

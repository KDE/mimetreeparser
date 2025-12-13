// SPDX-FileCopyrightText: 2003 Marc Mutz <mutz@kde.org>
// SPDX-License-Identifier: GPL-2.0-only

#include "mimetreeparser_core_debug.h"

#include "bodypartformatter.h"

#include "bodypartformatterbasefactory.h"
#include "bodypartformatterbasefactory_p.h"

#include "messagepart.h"
#include "objecttreeparser.h"
#include "utils.h"

#include <KMime/Content>
#include <QGpgME/Protocol>

using namespace MimeTreeParser;
using namespace MimeTreeParser::Interface;
using namespace Qt::Literals::StringLiterals;
namespace MimeTreeParser
{
class AnyTypeBodyPartFormatter : public MimeTreeParser::Interface::BodyPartFormatter
{
};

class MessageRfc822BodyPartFormatter : public MimeTreeParser::Interface::BodyPartFormatter
{
public:
    QSharedPointer<MessagePart> process(ObjectTreeParser *objectTreeParser, KMime::Content *node) const override
    {
        return QSharedPointer<MessagePart>(new EncapsulatedRfc822MessagePart(objectTreeParser, node, node->bodyAsMessage()));
    }
};

class HeadersBodyPartFormatter : public MimeTreeParser::Interface::BodyPartFormatter
{
public:
    QSharedPointer<MessagePart> process(ObjectTreeParser *objectTreeParser, KMime::Content *node) const override
    {
        return QSharedPointer<MessagePart>(new HeadersPart(objectTreeParser, node));
    }
};

class MultiPartRelatedBodyPartFormatter : public MimeTreeParser::Interface::BodyPartFormatter
{
public:
    QList<QSharedPointer<MessagePart>> processList(ObjectTreeParser *objectTreeParser, KMime::Content *node) const override
    {
        if (node->contents().isEmpty()) {
            return {};
        }
        // We rely on the order of the parts.
        // Theoretically there could also be a Start parameter which would break this..
        // https://tools.ietf.org/html/rfc2387#section-4

        // We want to display attachments even if displayed inline.
        QList<QSharedPointer<MessagePart>> list;
        list.append(QSharedPointer<MimeMessagePart>(new MimeMessagePart(objectTreeParser, node->contents().at(0), true)));
        for (int i = 1; i < node->contents().size(); i++) {
            auto p = node->contents().at(i);
            if (KMime::isAttachment(p)) {
                list.append(QSharedPointer<MimeMessagePart>(new MimeMessagePart(objectTreeParser, p, true)));
            }
        }
        return list;
    }
};

class MultiPartMixedBodyPartFormatter : public MimeTreeParser::Interface::BodyPartFormatter
{
public:
    QSharedPointer<MessagePart> process(ObjectTreeParser *objectTreeParser, KMime::Content *node) const override
    {
        auto contents = node->contents();
        if (contents.isEmpty()) {
            return {};
        }

        bool isActuallyMixedEncrypted = false;
        for (const auto &content : std::as_const(contents)) {
            if (content->contentType()->mimeType() == "application/pgp-encrypted"_ba || content->contentType()->mimeType() == "application/pkcs7-mime"_ba) {
                isActuallyMixedEncrypted = true;
            }
        }

        if (isActuallyMixedEncrypted) {
            // Remove explaination
            contents.erase(std::remove_if(contents.begin(),
                                          contents.end(),
                                          [](const auto content) {
                                              return content->contentType()->mimeType() == "text/plain";
                                          }),
                           contents.end());

            if (contents.count() == 1 && contents[0]->contentType()->mimeType() == "application/pkcs7-mime"_ba) {
                auto data = findTypeInDirectChildren(node, "application/pkcs7-mime"_ba);
                auto mp = QSharedPointer<EncryptedMessagePart>(new EncryptedMessagePart(objectTreeParser, data->decodedText(), QGpgME::smime(), node, data));
                mp->setIsEncrypted(true);
                return mp;
            }

            if (contents.count() == 2 && contents[1]->contentType()->mimeType() == "application/octet-stream"_ba) {
                KMime::Content *data = findTypeInDirectChildren(node, "application/octet-stream"_ba);
                auto mp = QSharedPointer<EncryptedMessagePart>(new EncryptedMessagePart(objectTreeParser, data->decodedText(), QGpgME::openpgp(), node, data));
                mp->setIsEncrypted(true);
                return mp;
            }
        }

        // we need the intermediate part to preserve the headers (necessary for with protected headers using multipart mixed)
        auto part = QSharedPointer<MessagePart>(new MessagePart(objectTreeParser, {}, node));
        part->appendSubPart(QSharedPointer<MimeMessagePart>(new MimeMessagePart(objectTreeParser, contents.at(0), false)));
        return part;
    }
};

class ApplicationPGPEncryptedBodyPartFormatter : public MimeTreeParser::Interface::BodyPartFormatter
{
public:
    QSharedPointer<MessagePart> process(ObjectTreeParser *objectTreeParser, KMime::Content *node) const override
    {
        if (node->decodedBody().trimmed() != "Version: 1"_ba) {
            qCWarning(MIMETREEPARSER_CORE_LOG) << "Unknown PGP Version String:" << node->decodedBody().trimmed();
        }

        if (!node->parent()) {
            return QSharedPointer<MessagePart>();
        }

        KMime::Content *data = findTypeInDirectChildren(node->parent(), "application/octet-stream"_ba);

        if (!data) {
            return QSharedPointer<MessagePart>(); // new MimeMessagePart(objectTreeParser, node));
        }

        QSharedPointer<EncryptedMessagePart> mp(new EncryptedMessagePart(objectTreeParser, data->decodedText(), QGpgME::openpgp(), node, data));
        mp->setIsEncrypted(true);
        return mp;
    }
};

class ApplicationPkcs7MimeBodyPartFormatter : public MimeTreeParser::Interface::BodyPartFormatter
{
public:
    QSharedPointer<MessagePart> process(ObjectTreeParser *objectTreeParser, KMime::Content *node) const override
    {
        if (node->head().isEmpty()) {
            return QSharedPointer<MessagePart>();
        }

        const QString smimeType = node->contentType()->parameter("smime-type").toLower();

        if (smimeType == QLatin1StringView("certs-only")) {
            return QSharedPointer<CertMessagePart>(new CertMessagePart(objectTreeParser, node, QGpgME::smime()));
        }

        bool isSigned = (smimeType == QLatin1StringView("signed-data"));
        bool isEncrypted = (smimeType == QLatin1StringView("enveloped-data"));

        // Analyze "signTestNode" node to find/verify a signature.
        // If zero part.objectTreeParser verification was successfully done after
        // decrypting via recursion by insertAndParseNewChildNode().
        KMime::Content *signTestNode = isEncrypted ? nullptr : node;

        // We try decrypting the content
        // if we either *know* that it is an encrypted message part
        // or there is neither signed nor encrypted parameter.
        QSharedPointer<MessagePart> mp;
        if (!isSigned) {
            if (isEncrypted) {
                qCDebug(MIMETREEPARSER_CORE_LOG) << "pkcs7 mime     ==      S/MIME TYPE: enveloped (encrypted) data";
            } else {
                qCDebug(MIMETREEPARSER_CORE_LOG) << "pkcs7 mime  -  type unknown  -  enveloped (encrypted) data ?";
            }

            auto _mp = QSharedPointer<EncryptedMessagePart>(new EncryptedMessagePart(objectTreeParser, node->decodedText(), QGpgME::smime(), node));
            mp = _mp;
            _mp->setIsEncrypted(true);
            // PartMetaData *messagePart(_mp->partMetaData());
            // if (!part.source()->decryptMessage()) {
            // isEncrypted = true;
            signTestNode = nullptr; // PENDING(marc) to be abs. sure, we'd need to have to look at the content
            // } else {
            //     _mp->startDecryption();
            //     if (messagePart->isDecryptable) {
            //         qCDebug(MIMETREEPARSER_CORE_LOG) << "pkcs7 mime  -  encryption found  -  enveloped (encrypted) data !";
            //         isEncrypted = true;
            //         part.nodeHelper()->setEncryptionState(node, KMMsgFullyEncrypted);
            //         signTestNode = nullptr;

            //     } else {
            //         // decryption failed, which could be because the part was encrypted but
            //         // decryption failed, or because we didn't know if it was encrypted, tried,
            //         // and failed. If the message was not actually encrypted, we continue
            //         // assuming it's signed
            //         if (_mp->passphraseError() || (smimeType.isEmpty() && messagePart->isEncrypted)) {
            //             isEncrypted = true;
            //             signTestNode = nullptr;
            //         }

            //         if (isEncrypted) {
            //             qCDebug(MIMETREEPARSER_CORE_LOG) << "pkcs7 mime  -  ERROR: COULD NOT DECRYPT enveloped data !";
            //         } else {
            //             qCDebug(MIMETREEPARSER_CORE_LOG) << "pkcs7 mime  -  NO encryption found";
            //         }
            //     }
            // }
        }

        // We now try signature verification if necessarry.
        if (signTestNode) {
            if (isSigned) {
                qCDebug(MIMETREEPARSER_CORE_LOG) << "pkcs7 mime     ==      S/MIME TYPE: opaque signed data";
            } else {
                qCDebug(MIMETREEPARSER_CORE_LOG) << "pkcs7 mime  -  type unknown  -  opaque signed data ?";
            }

            return QSharedPointer<SignedMessagePart>(new SignedMessagePart(objectTreeParser, QGpgME::smime(), nullptr, signTestNode));
        }
        return mp;
    }
};

class MultiPartAlternativeBodyPartFormatter : public MimeTreeParser::Interface::BodyPartFormatter
{
public:
    QSharedPointer<MessagePart> process(ObjectTreeParser *objectTreeParser, KMime::Content *node) const override
    {
        if (node->contents().isEmpty()) {
            return QSharedPointer<MessagePart>();
        }

        QSharedPointer<AlternativeMessagePart> mp(new AlternativeMessagePart(objectTreeParser, node));
        if (mp->mChildParts.isEmpty()) {
            return QSharedPointer<MimeMessagePart>(new MimeMessagePart(objectTreeParser, node->contents().at(0)));
        }
        return mp;
    }
};

class MultiPartEncryptedBodyPartFormatter : public MimeTreeParser::Interface::BodyPartFormatter
{
public:
    QSharedPointer<MessagePart> process(ObjectTreeParser *objectTreeParser, KMime::Content *node) const override
    {
        if (node->contents().isEmpty()) {
            Q_ASSERT(false);
            return QSharedPointer<MessagePart>();
        }

        const QGpgME::Protocol *protocol = nullptr;

        /*
        ATTENTION: This code is to be replaced by the new 'auto-detect' feature. --------------------------------------
        */
        KMime::Content *data = findTypeInDirectChildren(node, "application/octet-stream"_ba);
        if (data) {
            protocol = QGpgME::openpgp();
        } else {
            data = findTypeInDirectChildren(node, "application/pkcs7-mime"_ba);
            if (data) {
                protocol = QGpgME::smime();
            }
        }
        /*
        ---------------------------------------------------------------------------------------------------------------
        */

        if (!data) {
            return QSharedPointer<MessagePart>(new MimeMessagePart(objectTreeParser, node->contents().at(0)));
        }

        QSharedPointer<EncryptedMessagePart> mp(new EncryptedMessagePart(objectTreeParser, data->decodedText(), protocol, node, data));
        mp->setIsEncrypted(true);
        return mp;
    }
};

class MultiPartSignedBodyPartFormatter : public MimeTreeParser::Interface::BodyPartFormatter
{
public:
    static const QGpgME::Protocol *detectProtocol(const QString &protocolContentType_, const QString &signatureContentType)
    {
        auto protocolContentType = protocolContentType_;
        if (protocolContentType.isEmpty()) {
            qCWarning(MIMETREEPARSER_CORE_LOG) << "Message doesn't set the protocol for the multipart/signed content-type, "
                                                  "using content-type of the signature:"
                                               << signatureContentType;
            protocolContentType = signatureContentType;
        }

        const QGpgME::Protocol *protocol = nullptr;
        if (protocolContentType == QLatin1StringView("application/pkcs7-signature")
            || protocolContentType == QLatin1StringView("application/x-pkcs7-signature")) {
            protocol = QGpgME::smime();
        } else if (protocolContentType == QLatin1StringView("application/pgp-signature")
                   || protocolContentType == QLatin1StringView("application/x-pgp-signature")) {
            protocol = QGpgME::openpgp();
        }
        return protocol;
    }

    QSharedPointer<MessagePart> process(ObjectTreeParser *objectTreeParser, KMime::Content *node) const override
    {
        if (node->contents().size() != 2) {
            qCDebug(MIMETREEPARSER_CORE_LOG) << "mulitpart/signed must have exactly two child parts!" << Qt::endl << "processing as multipart/mixed";
            if (!node->contents().isEmpty()) {
                return QSharedPointer<MessagePart>(new MimeMessagePart(objectTreeParser, node->contents().at(0)));
            } else {
                return QSharedPointer<MessagePart>();
            }
        }

        KMime::Content *signedData = node->contents().at(0);
        KMime::Content *signature = node->contents().at(1);
        Q_ASSERT(signedData);
        Q_ASSERT(signature);

        auto protocol = detectProtocol(node->contentType()->parameter("protocol").toLower(), QLatin1StringView(signature->contentType()->mimeType().toLower()));

        if (!protocol) {
            return QSharedPointer<MessagePart>(new MimeMessagePart(objectTreeParser, signedData));
        }

        return QSharedPointer<SignedMessagePart>(new SignedMessagePart(objectTreeParser, protocol, signature, signedData));
    }
};

class TextHtmlBodyPartFormatter : public MimeTreeParser::Interface::BodyPartFormatter
{
public:
    QSharedPointer<MessagePart> process(ObjectTreeParser *objectTreeParser, KMime::Content *node) const override
    {
        return QSharedPointer<HtmlMessagePart>(new HtmlMessagePart(objectTreeParser, node));
    }
};

class TextPlainBodyPartFormatter : public MimeTreeParser::Interface::BodyPartFormatter
{
public:
    QSharedPointer<MessagePart> process(ObjectTreeParser *objectTreeParser, KMime::Content *node) const override
    {
        if (KMime::isAttachment(node)) {
            return QSharedPointer<AttachmentMessagePart>(new AttachmentMessagePart(objectTreeParser, node));
        }
        return QSharedPointer<TextMessagePart>(new TextMessagePart(objectTreeParser, node));
    }
};

} // anon namespace

void BodyPartFormatterBaseFactoryPrivate::messageviewer_create_builtin_bodypart_formatters()
{
    auto any = new AnyTypeBodyPartFormatter;
    auto textPlain = new TextPlainBodyPartFormatter;
    auto pkcs7 = new ApplicationPkcs7MimeBodyPartFormatter;
    auto pgp = new ApplicationPGPEncryptedBodyPartFormatter;
    auto html = new TextHtmlBodyPartFormatter;
    auto headers = new HeadersBodyPartFormatter;
    auto multipartAlternative = new MultiPartAlternativeBodyPartFormatter;
    auto multipartMixed = new MultiPartMixedBodyPartFormatter;
    auto multipartSigned = new MultiPartSignedBodyPartFormatter;
    auto multipartEncrypted = new MultiPartEncryptedBodyPartFormatter;
    auto message = new MessageRfc822BodyPartFormatter;
    auto multipartRelated = new MultiPartRelatedBodyPartFormatter;

    insert("application", "octet-stream", any);
    insert("application", "pgp", textPlain);
    insert("application", "pkcs7-mime", pkcs7);
    insert("application", "x-pkcs7-mime", pkcs7);
    insert("application", "pgp-encrypted", pgp);
    insert("application", "*", any);

    insert("text", "html", html);
    insert("text", "rtf", any);
    insert("text", "plain", textPlain);
    insert("text", "rfc822-headers", headers);
    insert("text", "*", textPlain);

    insert("image", "*", any);

    insert("message", "rfc822", message);
    insert("message", "*", any);

    insert("multipart", "alternative", multipartAlternative);
    insert("multipart", "encrypted", multipartEncrypted);
    insert("multipart", "signed", multipartSigned);
    insert("multipart", "related", multipartRelated);
    insert("multipart", "*", multipartMixed);
    insert("*", "*", any);
}

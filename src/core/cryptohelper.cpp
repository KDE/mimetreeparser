// SPDX-FileCopyrightText: 2001,2002 the KPGP authors
// SPDX-FileCopyrightText: 2015 Sandro Knauß <knauss@kolabsys.com>
// SPDX-FileCopyrightText: 2017 Daniel Vrátil <dvratil@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "cryptohelper.h"

#include "mimetreeparser_core_debug.h"

#include <QGpgME/DecryptJob>
#include <QGpgME/Protocol>
#include <QGpgME/VerifyOpaqueJob>

#include <gpgme++/context.h>
#include <gpgme++/decryptionresult.h>
#include <gpgme++/verificationresult.h>

using namespace MimeTreeParser;

PGPBlockType Block::determineType() const
{
    const QByteArray data = text();
    if (data.startsWith("-----BEGIN PGP PUBLIC KEY BLOCK-----")) {
        return NoPgpBlock;
    } else if (data.startsWith("-----BEGIN PGP SIGNED")) {
        return ClearsignedBlock;
    } else if (data.startsWith("-----BEGIN PGP SIGNATURE")) {
        return SignatureBlock;
    } else if (data.startsWith("-----BEGIN PGP PUBLIC")) {
        return PublicKeyBlock;
    } else if (data.startsWith("-----BEGIN PGP PRIVATE") || data.startsWith("-----BEGIN PGP SECRET")) {
        return PrivateKeyBlock;
    } else if (data.startsWith("-----BEGIN PGP MESSAGE")) {
        if (data.startsWith("-----BEGIN PGP MESSAGE PART")) {
            return MultiPgpMessageBlock;
        } else {
            return PgpMessageBlock;
        }
    } else if (data.startsWith("-----BEGIN PGP ARMORED FILE")) {
        return PgpMessageBlock;
    } else if (data.startsWith("-----BEGIN PGP ")) {
        return UnknownBlock;
    } else {
        return NoPgpBlock;
    }
}

QVector<Block> MimeTreeParser::prepareMessageForDecryption(const QByteArray &msg)
{
    PGPBlockType pgpBlock = NoPgpBlock;
    QVector<Block> blocks;
    int start = -1; // start of the current PGP block
    int lastEnd = -1; // end of the last PGP block
    const int length = msg.length();

    if (msg.isEmpty()) {
        return blocks;
    }
    if (msg.startsWith("-----BEGIN PGP PUBLIC KEY BLOCK-----")) {
        return blocks;
    }

    if (msg.startsWith("-----BEGIN PGP ")) {
        start = 0;
    } else {
        start = msg.indexOf("\n-----BEGIN PGP ") + 1;
        if (start == 0) {
            blocks.append(Block(msg, NoPgpBlock));
            return blocks;
        }
    }

    while (start != -1) {
        int nextEnd;
        int nextStart;

        // is the PGP block a clearsigned block?
        if (!strncmp(msg.constData() + start + 15, "SIGNED", 6)) {
            pgpBlock = ClearsignedBlock;
        } else {
            pgpBlock = UnknownBlock;
        }

        nextEnd = msg.indexOf("\n-----END PGP ", start + 15);
        nextStart = msg.indexOf("\n-----BEGIN PGP ", start + 15);

        if (nextEnd == -1) { // Missing END PGP line
            if (lastEnd != -1) {
                blocks.append(Block(msg.mid(lastEnd + 1), UnknownBlock));
            } else {
                blocks.append(Block(msg.mid(start), UnknownBlock));
            }
            break;
        }

        if ((nextStart == -1) || (nextEnd < nextStart) || (pgpBlock == ClearsignedBlock)) {
            // most likely we found a PGP block (but we don't check if it's valid)

            // store the preceding non-PGP block
            if (start - lastEnd - 1 > 0) {
                blocks.append(Block(msg.mid(lastEnd + 1, start - lastEnd - 1), NoPgpBlock));
            }

            lastEnd = msg.indexOf("\n", nextEnd + 14);
            if (lastEnd == -1) {
                if (start < length) {
                    blocks.append(Block(msg.mid(start)));
                }
                break;
            } else {
                blocks.append(Block(msg.mid(start, lastEnd + 1 - start)));
                if ((nextStart != -1) && (nextEnd > nextStart)) {
                    nextStart = msg.indexOf("\n-----BEGIN PGP ", lastEnd + 1);
                }
            }
        }

        start = nextStart;

        if (start == -1) {
            if (lastEnd + 1 < length) {
                // rest of mail is no PGP Block
                blocks.append(Block(msg.mid(lastEnd + 1), NoPgpBlock));
            }
            break;
        } else {
            start++; // move start behind the '\n'
        }
    }

    return blocks;
}

Block::Block() = default;

Block::Block(const QByteArray &m)
    : msg(m)
{
    mType = determineType();
}

Block::Block(const QByteArray &m, PGPBlockType t)
    : msg(m)
    , mType(t)
{
}

QByteArray MimeTreeParser::Block::text() const
{
    return msg;
}

PGPBlockType Block::type() const
{
    return mType;
}

namespace
{

bool isPGP(const KMime::Content *part, bool allowOctetStream = false)
{
    const auto ct = static_cast<KMime::Headers::ContentType *>(part->headerByType("Content-Type"));
    return ct && (ct->isSubtype("pgp-encrypted") || ct->isSubtype("encrypted") || (allowOctetStream && ct->isMimeType("application/octet-stream")));
}

bool isSMIME(const KMime::Content *part)
{
    const auto ct = static_cast<KMime::Headers::ContentType *>(part->headerByType("Content-Type"));
    return ct && (ct->isSubtype("pkcs7-mime") || ct->isSubtype("x-pkcs7-mime"));
}

void copyHeader(const KMime::Headers::Base *header, KMime::Message::Ptr msg)
{
    auto newHdr = KMime::Headers::createHeader(header->type());
    if (!newHdr) {
        newHdr = new KMime::Headers::Generic(header->type());
    }
    newHdr->from7BitString(header->as7BitString(false));
    msg->appendHeader(newHdr);
}

bool isContentHeader(const KMime::Headers::Base *header)
{
    return header->is("Content-Type") || header->is("Content-Transfer-Encoding") || header->is("Content-Disposition");
}

KMime::Message::Ptr assembleMessage(const KMime::Message::Ptr &orig, const KMime::Content *newContent)
{
    auto out = KMime::Message::Ptr::create();
    // Use the new content as message content
    out->setBody(const_cast<KMime::Content *>(newContent)->encodedBody());
    out->parse();

    // remove default explicit content headers added by KMime::Content::parse()
    QVector<KMime::Headers::Base *> headers = out->headers();
    for (const auto hdr : std::as_const(headers)) {
        if (isContentHeader(hdr)) {
            out->removeHeader(hdr->type());
        }
    }

    // Copy over headers from the original message, except for CT, CTE and CD
    // headers, we want to preserve those from the new content
    headers = orig->headers();
    for (const auto hdr : std::as_const(headers)) {
        if (isContentHeader(hdr)) {
            continue;
        }

        copyHeader(hdr, out);
    }

    // Overwrite some headers by those provided by the new content
    headers = newContent->headers();
    for (const auto hdr : std::as_const(headers)) {
        if (isContentHeader(hdr)) {
            copyHeader(hdr, out);
        }
    }

    out->assemble();
    out->parse();

    return out;
}
}

KMime::Message::Ptr CryptoUtils::decryptMessage(const KMime::Message::Ptr &msg, bool &wasEncrypted)
{
    GpgME::Protocol protoName = GpgME::UnknownProtocol;
    bool multipart = false;
    if (msg->contentType(false) && msg->contentType(false)->isMimeType("multipart/encrypted")) {
        multipart = true;
        const auto subparts = msg->contents();
        for (auto subpart : subparts) {
            if (isPGP(subpart, true)) {
                protoName = GpgME::OpenPGP;
                break;
            } else if (isSMIME(subpart)) {
                protoName = GpgME::CMS;
                break;
            }
        }
    } else {
        if (isPGP(msg.data())) {
            protoName = GpgME::OpenPGP;
        } else if (isSMIME(msg.data())) {
            protoName = GpgME::CMS;
        } else {
            const auto blocks = prepareMessageForDecryption(msg->body());
            QByteArray content;
            for (const auto &block : blocks) {
                if (block.type() == PgpMessageBlock) {
                    const auto proto = QGpgME::openpgp();
                    wasEncrypted = true;
                    QByteArray outData;
                    auto inData = block.text();
                    auto decrypt = proto->decryptJob();
                    auto ctx = QGpgME::Job::context(decrypt);
                    ctx->setDecryptionFlags(GpgME::Context::DecryptUnwrap);
                    auto result = decrypt->exec(inData, outData);
                    if (result.error()) {
                        // unknown key, invalid algo, or general error
                        qCWarning(MIMETREEPARSER_CORE_LOG) << "Failed to decrypt:" << result.error().asString();
                        return {};
                    }

                    inData = outData;
                    auto verify = proto->verifyOpaqueJob(true);
                    auto resultVerify = verify->exec(inData, outData);
                    if (resultVerify.error()) {
                        qCWarning(MIMETREEPARSER_CORE_LOG) << "Failed to verify:" << resultVerify.error().asString();
                        return {};
                    }

                    content += KMime::CRLFtoLF(outData);
                } else if (block.type() == NoPgpBlock) {
                    content += block.text();
                }
            }

            KMime::Content decCt;
            decCt.setBody(content);
            decCt.parse();
            decCt.assemble();

            return assembleMessage(msg, &decCt);
        }
    }

    if (protoName == GpgME::UnknownProtocol) {
        // Not encrypted, or we don't recognize the encryption
        wasEncrypted = false;
        return {};
    }

    const auto proto = (protoName == GpgME::OpenPGP) ? QGpgME::openpgp() : QGpgME::smime();

    wasEncrypted = true;
    QByteArray outData;
    auto inData = multipart ? msg->encodedContent() : msg->decodedContent(); // decodedContent in fact returns decoded body
    auto decrypt = proto->decryptJob();
    auto result = decrypt->exec(inData, outData);
    if (result.error()) {
        // unknown key, invalid algo, or general error
        qCWarning(MIMETREEPARSER_CORE_LOG) << "Failed to decrypt:" << result.error().asString();
        return {};
    }

    KMime::Content decCt;
    decCt.setContent(KMime::CRLFtoLF(outData));
    decCt.parse();
    decCt.assemble();

    return assembleMessage(msg, &decCt);
}

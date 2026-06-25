// SPDX-FileCopyrightText: 2001,2002 the KPGP authors
// SPDX-FileCopyrightText: 2015 Sandro Knauß <knauss@kolabsys.com>
// SPDX-FileCopyrightText: 2017 Daniel Vrátil <dvratil@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "cryptohelper.h"

#include "mimetreeparser_core_debug.h"
#include "objecttreeparser.h"

#include <QGpgME/DecryptJob>
#include <QGpgME/DecryptVerifyJob>
#include <QGpgME/Protocol>

#include <Libkleo/Formatting>

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

QList<Block> MimeTreeParser::prepareMessageForDecryption(const QByteArray &msg)
{
    PGPBlockType pgpBlock = NoPgpBlock;
    QList<Block> blocks;
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

void copyHeader(const KMime::Headers::Base *header, KMime::Content *msg)
{
    auto newHdr = KMime::Headers::createHeader(header->type());
    if (!newHdr) {
        newHdr = std::make_unique<KMime::Headers::Generic>(header->type());
    }
    newHdr->from7BitString(header->as7BitString());
    msg->appendHeader(std::move(newHdr));
}

[[nodiscard]] bool isContentHeader(const KMime::Headers::Base *header)
{
    return header->is("Content-Type") || header->is("Content-Transfer-Encoding") || header->is("Content-Disposition");
}

////////////////// save decrpyted message - new implementation ////////////////////
// We rely on the representation of the decrypted mail as given by ObjectTreeParser.
// However to remain as close to the original message as possible, we save the underlying
// KMime::Content. Before we can do that, we need to replace any encrypted nodes with their
// decrypted representation. That is not entirely trivial due to (some) messages being
// mangled by (some) MTAs, so there are instances, where we cannot just replace the entire
// parent node of the encrypted data with the decrypted data. Here, we can detect such cases
// (which have been handled during parsing), by checking, which KMime-nodes have a MessagePart-
// representation. Those, we want to keep, the others we can strip.
/////////////////////////////////////////////////////////////////////

static QSet<KMime::Content *> representedNodes(MimeTreeParser::MessagePart *part)
{
    QSet<KMime::Content *> ret;
    ret.insert(part->node());
    for (const auto &p : part->subParts()) {
        ret.unite(representedNodes(p.get()));
    }
    return ret;
}

static void copyMissingHeaders(const KMime::Content *from, KMime::Content *to)
{
    const auto headers = from->headers();
    const auto newHeaders = to->headers();
    for (const auto hdr : headers) {
        if (!(isContentHeader(hdr) || to->hasHeader(hdr->type()))) {
            copyHeader(hdr, to);
        }
    }

    // this is not strictly necessary, but sort content type headers after
    // headers such as From: and To:
    for (const auto hdr : newHeaders) {
        if (isContentHeader(hdr)) {
            auto newHdr = KMime::Headers::createHeader(hdr->type());
            newHdr->from7BitString(hdr->as7BitString());
            to->removeHeader(hdr->type());
            to->appendHeader(std::move(newHdr));
        }
    }
}

static void decryptNodes(MimeTreeParser::MessagePart *part, const QSet<KMime::Content *> &nonRemovableNodes, bool &wasEncrypted, MessagePart::Error &error)
{
    if (auto enc = qobject_cast<MimeTreeParser::EncryptedMessagePart *>(part)) {
        wasEncrypted = true;
        if (enc->error()) {
            error = enc->error();
            if (error == MessagePart::UserCancelled) {
                // In other cases, a partial result may or may not be useful
                return;
            }
        } else {
            auto node = part->node();
            auto subnodes = node->contents();
            QList<KMime::Content *> subnodesToKeep;
            int gap = -1;
            for (auto sub : subnodes) {
                if (nonRemovableNodes.contains(sub)) {
                    subnodesToKeep.append(sub);
                } else {
                    gap = subnodesToKeep.size();
                }
            }

            if (subnodesToKeep.isEmpty()) {
                const auto oldnode = node->clone();
                node->setContent(enc->mDecryptedData);
                node->parse();
                copyMissingHeaders(oldnode.get(), node);
            } else {
                auto decryptedNode = new KMime::Content();
                decryptedNode->setContent(enc->mDecryptedData);
                subnodesToKeep.insert(gap, decryptedNode);
                for (auto subnode : subnodesToKeep) {
                    node->appendContent(std::unique_ptr<KMime::Content>(subnode));
                }
                decryptedNode->parse(); // Do this _after_ the node has a parent, and thereby depth()
            }
        }
    } else if (auto text = qobject_cast<MimeTreeParser::TextMessagePart *>(part)) {
        if (text->encryptionState()) {
            wasEncrypted = true;
        }
        if (!text->subParts().isEmpty()) {
            // this is a text/plain part with one or more inline PGP blocks
            // throw away the contents, and assemble from the subparts
            auto node = text->node();
            auto ct = node->contentType();
            QStringEncoder codec(ct->charset().constData());
            QByteArray encoded = codec.encode(text->text());
            if (codec.hasError()) {
                // original charset might no longer work after decryption
                ct->setCharset(QByteArrayLiteral("UTF-8"));
                encoded = text->text().toUtf8();
            }
            node->setBody(encoded);
            return;
        }
    }
    for (const auto &p : part->subParts()) {
        decryptNodes(p.get(), nonRemovableNodes, wasEncrypted, error);
    }
}

std::shared_ptr<KMime::Message> CryptoUtils::decryptMessage(const std::shared_ptr<KMime::Message> &message, bool &wasEncrypted, MessagePart::Error &error)
{
    auto decryptedCopy = std::shared_ptr<KMime::Message>(static_cast<KMime::Message *>(message->clone().release()));

    MimeTreeParser::ObjectTreeParser otp;
    otp.parseObjectTree(decryptedCopy.get());
    otp.decryptAndVerify();

    auto nonRemovableNodes = representedNodes(otp.parsedPart().get());
    decryptNodes(otp.parsedPart().get(), nonRemovableNodes, wasEncrypted, error);
    decryptedCopy->assemble();

    return decryptedCopy;
}

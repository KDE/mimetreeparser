// SPDX-FileCopyrightText: 2001,2002 the KPGP authors
// SPDX-FileCopyrightText: 2015 Sandro Knauß <knauss@kolabsys.com>
// SPDX-FileCopyrightText: 2017 Daniel Vrátil <dvratil@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "mimetreeparser_core_export.h"

#include <KMime/Message>
#include <QByteArray>
#include <QList>
#include <gpgme++/global.h>

namespace MimeTreeParser
{
enum PGPBlockType {
    UnknownBlock = -1, // BEGIN PGP ???
    NoPgpBlock = 0,
    PgpMessageBlock = 1, // BEGIN PGP MESSAGE
    MultiPgpMessageBlock = 2, // BEGIN PGP MESSAGE, PART X[/Y]
    SignatureBlock = 3, // BEGIN PGP SIGNATURE
    ClearsignedBlock = 4, // BEGIN PGP SIGNED MESSAGE
    PublicKeyBlock = 5, // BEGIN PGP PUBLIC KEY BLOCK
    PrivateKeyBlock = 6, // BEGIN PGP PRIVATE KEY BLOCK (PGP 2.x: ...SECRET...)
};
/*!
 * \class MimeTreeParser::Block
 * \inmodule MimeTreeParserCore
 * \inheaderfile MimeTreeParserCore/CryptoHelper
 */
class MIMETREEPARSER_CORE_EXPORT Block
{
public:
    /*!
     * \brief Constructs an empty Block
     */
    Block();
    /*!
     * \brief Constructs a Block with the given message
     * \param m The message data
     */
    explicit Block(const QByteArray &m);

    /*!
     * \brief Constructs a Block with the given message and type
     * \param m The message data
     * \param t The block type
     */
    Block(const QByteArray &m, PGPBlockType t);

    /*!
     * \brief Returns the message text
     * \return The message data
     */
    [[nodiscard]] QByteArray text() const;
    /*!
     * \brief Returns the block type
     * \return The PGP block type
     */
    [[nodiscard]] PGPBlockType type() const;
    /*!
     * \brief Determines the type of the message block
     * \return The determined PGP block type
     */
    [[nodiscard]] PGPBlockType determineType() const;

    QByteArray msg;
    PGPBlockType mType = UnknownBlock;
};

/*! Parses the given message and splits it into OpenPGP blocks and
    Non-OpenPGP blocks.
*/
[[nodiscard]] MIMETREEPARSER_CORE_EXPORT QList<Block> prepareMessageForDecryption(const QByteArray &msg);

namespace CryptoUtils
{
[[nodiscard]] MIMETREEPARSER_CORE_EXPORT std::shared_ptr<KMime::Message>
decryptMessage(const std::shared_ptr<KMime::Message> &decrypt, bool &wasEncrypted, GpgME::Protocol &protoName);
}

} // namespace MimeTreeParser

Q_DECLARE_TYPEINFO(MimeTreeParser::Block, Q_RELOCATABLE_TYPE);

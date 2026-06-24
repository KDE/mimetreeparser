// SPDX-FileCopyrightText: 2015 Sandro Knauß <knauss@kolabsys.com>
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

#include "cryptohelpertest.h"

#include <MimeTreeParserCore/CryptoHelper>
#include <MimeTreeParserCore/FileOpener>
#include <MimeTreeParserCore/ObjectTreeParser>

#include <QTest>

using namespace MimeTreeParser;
using namespace Qt::Literals::StringLiterals;

static QByteArray readMailFromFile(const QString &mailFile)
{
    QFile file(QLatin1StringView(MAIL_DATA_DIR) + QLatin1Char('/') + mailFile);
    if (!file.open(QIODevice::ReadOnly)) {
        Q_ASSERT(false);
    }
    Q_ASSERT(file.isOpen());
    return file.readAll();
}

void CryptoHelperTest::testPMFDEmpty()
{
    QCOMPARE(prepareMessageForDecryption("").count(), 0);
}

void CryptoHelperTest::testPMFDWithNoPGPBlock()
{
    const QByteArray text = "testblabla";
    const QList<Block> blocks = prepareMessageForDecryption(text);
    QCOMPARE(blocks.count(), 1);
    QCOMPARE(blocks[0].text(), text);
    QCOMPARE(blocks[0].type(), NoPgpBlock);
}

void CryptoHelperTest::testPGPBlockType()
{
    const QString blockText = QStringLiteral("text");
    const QString preString = QStringLiteral("before\n");
    for (int i = 1; i <= PrivateKeyBlock; ++i) {
        QString name;
        switch (i) {
        case PgpMessageBlock:
            name = QStringLiteral("MESSAGE");
            break;
        case MultiPgpMessageBlock:
            name = QStringLiteral("MESSAGE PART");
            break;
        case SignatureBlock:
            name = QStringLiteral("SIGNATURE");
            break;
        case ClearsignedBlock:
            name = QStringLiteral("SIGNED MESSAGE");
            break;
        case PublicKeyBlock:
            name = QStringLiteral("PUBLIC KEY BLOCK");
            break;
        case PrivateKeyBlock:
            name = QStringLiteral("PRIVATE KEY BLOCK");
            break;
        }
        QString text = QLatin1StringView("-----BEGIN PGP ") + name + QLatin1Char('\n') + blockText;
        QList<Block> blocks = prepareMessageForDecryption(preString.toLatin1() + text.toLatin1());
        QCOMPARE(blocks.count(), 1);
        QCOMPARE(blocks[0].type(), UnknownBlock);

        text += QLatin1StringView("\n-----END PGP ") + name + QLatin1Char('\n');
        blocks = prepareMessageForDecryption(preString.toLatin1() + text.toLatin1());
        QCOMPARE(blocks.count(), 2);
        QCOMPARE(blocks[1].text(), text.toLatin1());
        QCOMPARE(blocks[1].type(), static_cast<PGPBlockType>(i));
    }
}

void CryptoHelperTest::testDeterminePGPBlockType()
{
    const QString blockText = QStringLiteral("text");
    for (int i = 1; i <= PrivateKeyBlock; ++i) {
        QString name;
        switch (i) {
        case PgpMessageBlock:
            name = QStringLiteral("MESSAGE");
            break;
        case MultiPgpMessageBlock:
            name = QStringLiteral("MESSAGE PART");
            break;
        case SignatureBlock:
            name = QStringLiteral("SIGNATURE");
            break;
        case ClearsignedBlock:
            name = QStringLiteral("SIGNED MESSAGE");
            break;
        case PublicKeyBlock:
            name = QStringLiteral("PUBLIC KEY BLOCK");
            break;
        case PrivateKeyBlock:
            name = QStringLiteral("PRIVATE KEY BLOCK");
            break;
        }
        const QString text = QLatin1StringView("-----BEGIN PGP ") + name + QLatin1Char('\n') + blockText + QLatin1Char('\n');
        const Block block = Block(text.toLatin1());
        QCOMPARE(block.text(), text.toLatin1());
        QCOMPARE(block.type(), static_cast<PGPBlockType>(i));
    }
}

void CryptoHelperTest::testEmbededPGPBlock()
{
    const QByteArray text = QByteArray("before\n-----BEGIN PGP MESSAGE-----\ncrypted - you see :)\n-----END PGP MESSAGE-----\nafter");
    const QList<Block> blocks = prepareMessageForDecryption(text);
    QCOMPARE(blocks.count(), 3);
    QCOMPARE(blocks[0].text(), QByteArray("before\n"));
    QCOMPARE(blocks[1].text(), QByteArray("-----BEGIN PGP MESSAGE-----\ncrypted - you see :)\n-----END PGP MESSAGE-----\n"));
    QCOMPARE(blocks[2].text(), QByteArray("after"));
}

void CryptoHelperTest::testClearSignedMessage()
{
    const QByteArray text = QByteArray(
        "before\n-----BEGIN PGP SIGNED MESSAGE-----\nsigned content\n-----BEGIN PGP SIGNATURE-----\nfancy signature\n-----END PGP SIGNATURE-----\nafter");
    const QList<Block> blocks = prepareMessageForDecryption(text);
    QCOMPARE(blocks.count(), 3);
    QCOMPARE(blocks[0].text(), QByteArray("before\n"));
    QCOMPARE(blocks[1].text(),
             QByteArray("-----BEGIN PGP SIGNED MESSAGE-----\nsigned content\n-----BEGIN PGP SIGNATURE-----\nfancy signature\n-----END PGP SIGNATURE-----\n"));
    QCOMPARE(blocks[2].text(), QByteArray("after"));
}

void CryptoHelperTest::testMultipleBlockMessage()
{
    const QByteArray text = QByteArray(
        "before\n-----BEGIN PGP SIGNED MESSAGE-----\nsigned content\n-----BEGIN PGP SIGNATURE-----\nfancy signature\n-----END PGP "
        "SIGNATURE-----\nafter\n-----BEGIN PGP MESSAGE-----\ncrypted - you see :)\n-----END PGP MESSAGE-----\n");
    const QList<Block> blocks = prepareMessageForDecryption(text);
    QCOMPARE(blocks.count(), 4);
    QCOMPARE(blocks[0].text(), QByteArray("before\n"));
    QCOMPARE(blocks[1].text(),
             QByteArray("-----BEGIN PGP SIGNED MESSAGE-----\nsigned content\n-----BEGIN PGP SIGNATURE-----\nfancy signature\n-----END PGP SIGNATURE-----\n"));
    QCOMPARE(blocks[2].text(), QByteArray("after\n"));
    QCOMPARE(blocks[3].text(), QByteArray("-----BEGIN PGP MESSAGE-----\ncrypted - you see :)\n-----END PGP MESSAGE-----\n"));
}

void CryptoHelperTest::testDecryptMessage()
{
    auto message = std::make_shared<KMime::Message>();
    message->setContent(readMailFromFile(QLatin1StringView("openpgp-encrypted+signed.mbox")));
    message->parse();

    bool wasEncrypted = false;
    MessagePart::Error error = MessagePart::NoError;
    auto decryptedMessage = CryptoUtils::decryptMessage(message, wasEncrypted, error);
    QVERIFY(wasEncrypted);
    QVERIFY(decryptedMessage);
    QCOMPARE(decryptedMessage->decodedBody(), QByteArray("encrypted message text"));
    QCOMPARE(decryptedMessage->encodedContent(),
             QByteArray("From test@kolab.org Wed, 08 Sep 2010 17: 02:52 +0200\nFrom: OpenPGP Test <test@kolab.org>\nTo: test@kolab.org\nSubject: OpenPGP "
                        "encrypted\nDate: Wed, 08 Sep 2010 17:02:52 +0200\nUser-Agent: KMail/4.6 pre (Linux/2.6.34-rc2-2-default; KDE/4.5.60; x86_64; ; "
                        ")\nMIME-Version: 1.0\nContent-Transfer-Encoding: 7Bit\nContent-Type: text/plain; charset=\"us-ascii\"\n\nencrypted message text"));
}

void CryptoHelperTest::testDecryptInlineMessage()
{
    auto message = std::make_shared<KMime::Message>();
    message->setContent(readMailFromFile(QLatin1StringView("openpgp-inline-encrypted+nonenc.mbox")));
    message->parse();

    bool wasEncrypted = false;
    MessagePart::Error error = MessagePart::NoError;
    auto decryptedMessage = CryptoUtils::decryptMessage(message, wasEncrypted, error);
    QVERIFY(wasEncrypted);
    QVERIFY(decryptedMessage);
    QCOMPARE(decryptedMessage->decodedBody(),
             QByteArray("Not encrypted not signed :(\n"
                        "\n"
                        "some random text\n"
                        "A signed part follows, but this text here is not signed, at all.\n"
                        "\n"
                        "asdasd asd asd asdf sadf sdaf sadf \xF6\xE4\xFC\nNot signed.\n"));
    QCOMPARE(decryptedMessage->encodedContent(),
             QByteArray("From test@kolab.org Wed, 25 May 2011 23: 49:40 +0100\n"
                        "From: OpenPGP Test <test@kolab.org>\n"
                        "To: test@kolab.org\n"
                        "Subject: inlinepgpencrypted + non enc text\n"
                        "Date: Wed, 25 May 2011 23:49:40 +0100\n"
                        "Message-ID: <1786696.yKXrOjjflF@herrwackelpudding.localhost>\n"
                        "X-KMail-Transport: GMX\n"
                        "X-KMail-Fcc: 28\n"
                        "X-KMail-Drafts: 7\n"
                        "X-KMail-Templates: 9\n"
                        "User-Agent: KMail/4.6 beta5 (Linux/2.6.34.7-0.7-desktop; KDE/4.6.41; x86_64;\n"
                        " git-0269848; 2011-04-19)\n"
                        "MIME-Version: 1.0\n"
                        "Content-Transfer-Encoding: 7Bit\n"
                        "Content-Type: text/plain; charset=\"iso-8859-1\"\n"
                        "\n"
                        "Not encrypted not signed :(\n"
                        "\n"
                        "some random text\n"
                        "A signed part follows, but this text here is not signed, at all.\n"
                        "\n"
                        "asdasd asd asd asdf sadf sdaf sadf \xF6\xE4\xFC\nNot signed.\n"));
}

static bool hasEncryptedContents(QSharedPointer<MimeTreeParser::MessagePart> part)
{
    const auto subparts = part->subParts();
    for (const auto &subpart : subparts) {
        if (!subpart->isAttachment() && hasEncryptedContents(subpart)) {
            return true;
        }
    }
    return part.dynamicCast<MimeTreeParser::EncryptedMessagePart>().get();
}

void CryptoHelperTest::testSaveDecrypted_data()
{
    QTest::addColumn<QString>("mailFile");
    QTest::addColumn<bool>("hasInlinePGPBlocks");
    // These are just a random selection of encrypted files in various formats. We check that
    // the version as returned by CryptoUtils::decryptMessage() parses to the same contents
    QTest::newRow("smime-encrypted") << "smime-encrypted.mbox" << false;
    QTest::newRow("openpgp-encrypted-attachment-and-non-encrypted-attachment") << "openpgp-encrypted-attachment-and-non-encrypted-attachment.mbox" << false;
    QTest::newRow("openpgp-encrypted-ambiguous-mime") << "openpgp-encrypted-ambiguous-mime.mbox" << false;
    QTest::newRow("openpgp-inline-charset-encrypted") << "openpgp-inline-charset-encrypted.mbox" << true;
    QTest::newRow("multipart-mixed-alternative-inline-pgp") << "multipart-mixed-alternative-inline-pgp.mbox" << true;
    QTest::newRow("openpgp-inline-encrypted+nonenc") << "openpgp-inline-encrypted+nonenc.mbox" << true;
    QTest::newRow("signed-forward-openpgp-signed-encrypted") << "signed-forward-openpgp-signed-encrypted.mbox" << false;
}

void CryptoHelperTest::testSaveDecrypted()
{
    QFETCH(QString, mailFile);
    QFETCH(bool, hasInlinePGPBlocks);
    const auto originalMessage = MimeTreeParser::Core::FileOpener::openFile(QLatin1StringView(MAIL_DATA_DIR) + u'/' + mailFile).value(0);

    MimeTreeParser::ObjectTreeParser originalOtp;
    originalOtp.parseObjectTree(originalMessage.get());
    originalOtp.decryptAndVerify();
    const auto originalContents = originalOtp.collectContentParts();
    QVERIFY(originalContents.size() >= 1);
    bool wasEncrypted = false;
    MessagePart::Error error = MessagePart::NoError;
    const auto decryptedMessage = CryptoUtils::decryptMessage(originalMessage, wasEncrypted, error);

    MimeTreeParser::ObjectTreeParser decryptedOtp;
    decryptedOtp.parseObjectTree(decryptedMessage.get());
    decryptedOtp.decryptAndVerify(); // we still need to verify signatures
    QVERIFY(!hasEncryptedContents(decryptedOtp.parsedPart()));
    QCOMPARE(originalOtp.collectAttachmentParts().size(), decryptedOtp.collectAttachmentParts().size());
    const auto decryptedContents = decryptedOtp.collectContentParts();
    if (hasInlinePGPBlocks) {
        QVERIFY(originalContents.size() >= decryptedContents.size());
        QCOMPARE(originalOtp.plainTextContent(), decryptedOtp.plainTextContent());
    } else {
        QCOMPARE(originalContents.size(), decryptedContents.size());
        for (int i = 0; i < originalContents.size(); ++i) {
            QCOMPARE(originalContents.value(i)->text(), decryptedContents.value(i)->text());
        }
    }
}

QTEST_GUILESS_MAIN(CryptoHelperTest)

#include "moc_cryptohelpertest.cpp"

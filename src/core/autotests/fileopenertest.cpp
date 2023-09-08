// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include <MimeTreeParserCore/FileOpener>

#include <QDebug>
#include <QTest>

using namespace MimeTreeParser::Core;

class FileOpenerTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void openSingleMboxTest()
    {
        const auto messages = FileOpener::openFile(QLatin1String(MAIL_DATA_DIR) + QLatin1Char('/') + QLatin1String("smime-opaque-enc+sign.mbox"));
        QCOMPARE(messages.count(), 1);
    }

    void openSingleCombinedTest()
    {
        const auto messages = FileOpener::openFile(QLatin1String(MAIL_DATA_DIR) + QLatin1Char('/') + QLatin1String("combined.mbox"));
        QCOMPARE(messages.count(), 3);
    }

    void openAscTest()
    {
        const auto messages = FileOpener::openFile(QLatin1String(MAIL_DATA_DIR) + QLatin1Char('/') + QLatin1String("msg.asc"));
        QCOMPARE(messages.count(), 1);
        auto message = messages[0];

        QCOMPARE(message->contentType()->mimeType(), "multipart/encrypted");
        QCOMPARE(message->contents().count(), 2);

        auto pgpPart = message->contents()[0];
        QCOMPARE(pgpPart->contentType()->mimeType(), "application/pgp-encrypted");

        auto octetStreamPart = message->contents()[1];
        QCOMPARE(octetStreamPart->contentType()->mimeType(), "application/octet-stream");
    }

    void openSmimeTest()
    {
        const auto messages = FileOpener::openFile(QLatin1String(MAIL_DATA_DIR) + QLatin1Char('/') + QLatin1String("smime.p7m"));
        QCOMPARE(messages.count(), 1);
        auto message = messages[0];

        QCOMPARE(message->contentType()->mimeType(), "application/pkcs7-mime");
        QCOMPARE(message->contentType()->parameter(QStringLiteral("smime-type")), QStringLiteral("enveloped-data"));
        QCOMPARE(message->contentDisposition()->filename(), QStringLiteral("smime.p7m"));
    }
};

QTEST_GUILESS_MAIN(FileOpenerTest)
#include "fileopenertest.moc"

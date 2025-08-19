// SPDX-FileCopyrightText: 2016 Sandro Knauß <knauss@kolabsystems.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "partmodel.h"
#include <MimeTreeParserCore/ObjectTreeParser>

#include <QTest>
#include <QTimeZone>

using namespace Qt::Literals::StringLiterals;

static QByteArray readMailFromFile(const QString &mailFile)
{
    QFile file(QLatin1StringView(MAIL_DATA_DIR) + u'/' + mailFile);
    file.open(QIODevice::ReadOnly);
    Q_ASSERT(file.isOpen());
    return file.readAll();
}

class MimeTreeParserTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testTextMail()
    {
        const auto expectedText =
            u"If you can see this text it means that your email client couldn't display our newsletter properly.\nPlease visit this link to view the "
            u"newsletter "
            "on our website: http://www.gog.com/newsletter/"_s;
        MimeTreeParser::ObjectTreeParser otp;
        otp.parseObjectTree(readMailFromFile("plaintext.mbox"_L1));
        auto partList = otp.collectContentParts();
        QCOMPARE(partList.size(), 1);
        auto part = partList[0].dynamicCast<MimeTreeParser::MessagePart>();
        QCOMPARE(part->text(), expectedText);
        QCOMPARE(part->charset(), u"utf-8"_s.toLocal8Bit());

        QCOMPARE(part->encryptions().size(), 0);
        QCOMPARE(part->signatures().size(), 0);

        QCOMPARE(otp.collectAttachmentParts().size(), 0);

        QCOMPARE(otp.plainTextContent(), expectedText);
        QVERIFY(otp.htmlContent().isEmpty());
    }

    void testAlternative()
    {
        MimeTreeParser::ObjectTreeParser otp;
        otp.parseObjectTree(readMailFromFile("alternative.mbox"_L1));
        auto partList = otp.collectContentParts();
        QCOMPARE(partList.size(), 1);
        auto part = partList[0].dynamicCast<MimeTreeParser::AlternativeMessagePart>();
        QVERIFY(bool(part));
        QCOMPARE(part->plaintextContent(),
                 u"If you can see this text it means that your email client couldn't display our newsletter properly.\nPlease visit this link to "
                 "view the newsletter on our website: http://www.gog.com/newsletter/\n"_s);
        QCOMPARE(part->charset(), u"us-ascii"_s.toLocal8Bit());
        QCOMPARE(part->htmlContent(), u"<html><body><p><span>HTML</span> text</p></body></html>\n\n"_s);
        QCOMPARE(otp.collectAttachmentParts().size(), 0);
        QCOMPARE(part->encryptions().size(), 0);
        QCOMPARE(part->signatures().size(), 0);
    }

    void testTextHtml()
    {
        auto expectedText = u"<html><body><p><span>HTML</span> text</p></body></html>"_s;
        MimeTreeParser::ObjectTreeParser otp;
        otp.parseObjectTree(readMailFromFile("html.mbox"_L1));
        otp.print();
        auto partList = otp.collectContentParts();
        QCOMPARE(partList.size(), 1);
        auto part = partList[0].dynamicCast<MimeTreeParser::HtmlMessagePart>();
        QVERIFY(bool(part));
        QCOMPARE(part->htmlContent(), expectedText);
        QCOMPARE(part->charset(), u"windows-1252"_s.toLocal8Bit());
        QCOMPARE(part->encryptions().size(), 0);
        QCOMPARE(part->signatures().size(), 0);
        auto contentAttachmentList = otp.collectAttachmentParts();
        QCOMPARE(contentAttachmentList.size(), 0);

        QCOMPARE(otp.htmlContent(), expectedText);
        QVERIFY(otp.plainTextContent().isEmpty());
    }

    void testSMimeEncrypted()
    {
        MimeTreeParser::ObjectTreeParser otp;
        otp.parseObjectTree(readMailFromFile("smime-encrypted.mbox"_L1));
        otp.print();
        otp.decryptAndVerify();
        otp.print();
        auto partList = otp.collectContentParts();
        QCOMPARE(partList.size(), 1);
        auto part = partList[0].dynamicCast<MimeTreeParser::MessagePart>();
        QVERIFY(bool(part));
        QCOMPARE(part->text(), u"The quick brown fox jumped over the lazy dog."_s);
        QCOMPARE(part->charset(), u"us-ascii"_s.toLocal8Bit());
        QCOMPARE(part->encryptions().size(), 1);
        QCOMPARE(part->signatures().size(), 0);
        auto contentAttachmentList = otp.collectAttachmentParts();
        QCOMPARE(contentAttachmentList.size(), 0);
    }

    void testOpenPGPEncryptedAttachment()
    {
        MimeTreeParser::ObjectTreeParser otp;
        otp.parseObjectTree(readMailFromFile("openpgp-encrypted-attachment-and-non-encrypted-attachment.mbox"_L1));
        otp.print();
        otp.decryptAndVerify();
        otp.print();
        auto partList = otp.collectContentParts();
        QCOMPARE(partList.size(), 1);
        auto part = partList[0].dynamicCast<MimeTreeParser::MessagePart>();
        QVERIFY(bool(part));
        QCOMPARE(part->text(), u"test text"_s);
        QCOMPARE(part->charset(), u"us-ascii"_s.toLocal8Bit());
        QCOMPARE(part->encryptions().size(), 1);
        QCOMPARE(part->signatures().size(), 1);
        QCOMPARE(part->encryptionState(), MimeTreeParser::KMMsgFullyEncrypted);
        QCOMPARE(part->signatureState(), MimeTreeParser::KMMsgFullySigned);
        auto contentAttachmentList = otp.collectAttachmentParts();
        QCOMPARE(contentAttachmentList.size(), 2);
        //     QCOMPARE(contentAttachmentList[0]->availableContents(), QList<QByteArray>() << "text/plain");
        // QCOMPARE(contentAttachmentList[0]->content().size(), 1);
        QCOMPARE(contentAttachmentList[0]->encryptions().size(), 1);
        QCOMPARE(contentAttachmentList[0]->signatures().size(), 1);
        QCOMPARE(contentAttachmentList[0]->encryptionState(), MimeTreeParser::KMMsgFullyEncrypted);
        QCOMPARE(contentAttachmentList[0]->signatureState(), MimeTreeParser::KMMsgFullySigned);
        //     QCOMPARE(contentAttachmentList[1]->availableContents(), QList<QByteArray>() << "image/png");
        //     QCOMPARE(contentAttachmentList[1]->content().size(), 1);
        QCOMPARE(contentAttachmentList[1]->encryptions().size(), 0);
        QCOMPARE(contentAttachmentList[1]->signatures().size(), 0);
        QCOMPARE(contentAttachmentList[1]->encryptionState(), MimeTreeParser::KMMsgNotEncrypted);
        QCOMPARE(contentAttachmentList[1]->signatureState(), MimeTreeParser::KMMsgNotSigned);
    }

    void testOpenPGPInline()
    {
        MimeTreeParser::ObjectTreeParser otp;
        otp.parseObjectTree(readMailFromFile("openpgp-inline-charset-encrypted.mbox"_L1));
        otp.print();
        otp.decryptAndVerify();
        otp.print();
        auto partList = otp.collectContentParts();
        QCOMPARE(partList.size(), 1);
        auto part = partList[0].dynamicCast<MimeTreeParser::MessagePart>();
        QVERIFY(bool(part));
        QCOMPARE(part->charset(), u"ISO-8859-15"_s.toLocal8Bit());
        QCOMPARE(part->text(), QString::fromUtf8("asdasd asd asd asdf sadf sdaf sadf öäü"));

        QCOMPARE(part->encryptions().size(), 1);
        QCOMPARE(part->signatures().size(), 1);
        QCOMPARE(otp.collectAttachmentParts().size(), 0);
    }

    void testOpenPPGInlineWithNonEncText()
    {
        MimeTreeParser::ObjectTreeParser otp;
        otp.parseObjectTree(readMailFromFile("openpgp-inline-encrypted+nonenc.mbox"_L1));
        otp.print();
        otp.decryptAndVerify();
        otp.print();
        auto partList = otp.collectContentParts();
        QCOMPARE(partList.size(), 1);
        auto part1 = partList[0].dynamicCast<MimeTreeParser::MessagePart>();
        QVERIFY(bool(part1));
        QCOMPARE(part1->text(), u"Not encrypted not signed :(\n\nsome random text"_s);
        // TODO test if we get the proper subparts with the appropriate encryptions
        QCOMPARE(part1->charset(), u"us-ascii"_s.toLocal8Bit());

        QCOMPARE(part1->encryptionState(), MimeTreeParser::KMMsgPartiallyEncrypted);
        QCOMPARE(part1->signatureState(), MimeTreeParser::KMMsgNotSigned);

        // QCOMPARE(part1->text(), u"Not encrypted not signed :(\n\n"_s);
        // QCOMPARE(part1->charset(), u"us-ascii"_s.toLocal8Bit());
        // QCOMPARE(contentList[1]->content(), u"some random text"_s.toLocal8Bit());
        // QCOMPARE(contentList[1]->charset(), u"us-ascii"_s.toLocal8Bit());
        // QCOMPARE(contentList[1]->encryptions().size(), 1);
        // QCOMPARE(contentList[1]->signatures().size(), 0);
        QCOMPARE(otp.collectAttachmentParts().size(), 0);
    }

    void testEncryptionBlock()
    {
        MimeTreeParser::ObjectTreeParser otp;
        otp.parseObjectTree(readMailFromFile("openpgp-encrypted-attachment-and-non-encrypted-attachment.mbox"_L1));
        otp.print();
        otp.decryptAndVerify();
        otp.print();
        auto partList = otp.collectContentParts();
        QCOMPARE(partList.size(), 1);
        auto part1 = partList[0].dynamicCast<MimeTreeParser::MessagePart>();
        QVERIFY(bool(part1));
        QCOMPARE(part1->encryptions().size(), 1);
        //     auto enc = contentList[0]->encryptions()[0];
        //     QCOMPARE((int) enc->recipients().size(), 2);

        //     auto r = enc->recipients()[0];
        //     QCOMPARE(r->keyid(),u"14B79E26050467AA"_s);
        //     QCOMPARE(r->name(),u"kdetest"_s);
        //     QCOMPARE(r->email(),u"you@you.com"_s);
        //     QCOMPARE(r->comment(),u""_s);

        //     r = enc->recipients()[1];
        //     QCOMPARE(r->keyid(),u"8D9860C58F246DE6"_s);
        //     QCOMPARE(r->name(),u"unittest key"_s);
        //     QCOMPARE(r->email(),u"test@kolab.org"_s);
        //     QCOMPARE(r->comment(),u"no password"_s);
        auto attachmentList = otp.collectAttachmentParts();
        QCOMPARE(attachmentList.size(), 2);
        auto attachment1 = attachmentList[0];
        QVERIFY(attachment1->node());
        QCOMPARE(attachment1->filename(), u"file.txt"_s);
        auto attachment2 = attachmentList[1];
        QVERIFY(attachment2->node());
        QCOMPARE(attachment2->filename(), u"image.png"_s);
    }

    void testSignatureBlock()
    {
        MimeTreeParser::ObjectTreeParser otp;
        otp.parseObjectTree(readMailFromFile("openpgp-encrypted-attachment-and-non-encrypted-attachment.mbox"_L1));
        otp.print();
        otp.decryptAndVerify();
        otp.print();
        auto partList = otp.collectContentParts();
        QCOMPARE(partList.size(), 1);
        auto part = partList[0].dynamicCast<MimeTreeParser::MessagePart>();
        QVERIFY(bool(part));

        // QCOMPARE(contentList[0]->signatures().size(), 1);
        // auto sig = contentList[0]->signatures()[0];
        // QCOMPARE(sig->creationDateTime(), QDateTime(QDate(2015,05,01),QTime(15,12,47)));
        // QCOMPARE(sig->expirationDateTime(), QDateTime());
        // QCOMPARE(sig->neverExpires(), true);

        // auto key = sig->key();
        // QCOMPARE(key->keyid(),u"8D9860C58F246DE6"_s);
        // QCOMPARE(key->name(),u"unittest key"_s);
        // QCOMPARE(key->email(),u"test@kolab.org"_s);
        // QCOMPARE(key->comment(),u"no password"_s);
    }

    void testRelatedAlternative()
    {
        MimeTreeParser::ObjectTreeParser otp;
        otp.parseObjectTree(readMailFromFile("cid-links.mbox"_L1));
        otp.print();
        auto partList = otp.collectContentParts();
        QCOMPARE(partList.size(), 1);
        auto part = partList[0].dynamicCast<MimeTreeParser::MessagePart>();
        QVERIFY(bool(part));
        QCOMPARE(part->encryptions().size(), 0);
        QCOMPARE(part->signatures().size(), 0);
        QCOMPARE(otp.collectAttachmentParts().size(), 1);
    }

    void testAttachmentPart()
    {
        MimeTreeParser::ObjectTreeParser otp;
        otp.parseObjectTree(readMailFromFile("attachment.mbox"_L1));
        otp.print();
        auto partList = otp.collectAttachmentParts();
        QCOMPARE(partList.size(), 1);
        auto part = partList[0].dynamicCast<MimeTreeParser::MessagePart>();
        QVERIFY(bool(part));
        QCOMPARE(part->mimeType(), "image/jpeg");
        QCOMPARE(part->filename(), u"aqnaozisxya.jpeg"_s);
    }

    void testAttachment2Part()
    {
        MimeTreeParser::ObjectTreeParser otp;
        otp.parseObjectTree(readMailFromFile("attachment2.mbox"_L1));
        otp.print();
        auto partList = otp.collectAttachmentParts();
        QCOMPARE(partList.size(), 1);
        auto part = partList[0].dynamicCast<MimeTreeParser::MessagePart>();
        QVERIFY(bool(part));
        QCOMPARE(part->mimeType(), "image/jpeg");
        QCOMPARE(part->filename(), u"aqnaozisxya.jpeg"_s);
    }

    void testCidLink()
    {
        MimeTreeParser::ObjectTreeParser otp;
        otp.parseObjectTree(readMailFromFile("cid-links.mbox"_L1));
        otp.print();
        auto partList = otp.collectContentParts();
        QCOMPARE(partList.size(), 1);
        auto part = partList[0].dynamicCast<MimeTreeParser::AlternativeMessagePart>();
        QVERIFY(bool(part));
        auto resolvedContent = otp.resolveCidLinks(part->htmlContent());
        QVERIFY(!resolvedContent.contains("cid:"_L1));
    }

    void testCidLinkInForwardedInline()
    {
        MimeTreeParser::ObjectTreeParser otp;
        otp.parseObjectTree(readMailFromFile("cid-links-forwarded-inline.mbox"_L1));
        otp.print();
        auto partList = otp.collectContentParts();
        QCOMPARE(partList.size(), 1);
        auto part = partList[0].dynamicCast<MimeTreeParser::AlternativeMessagePart>();
        QVERIFY(bool(part));
        auto resolvedContent = otp.resolveCidLinks(part->htmlContent());
        QVERIFY(!resolvedContent.contains("cid:"_L1));
    }

    void testOpenPGPInlineError()
    {
        MimeTreeParser::ObjectTreeParser otp;
        otp.parseObjectTree(readMailFromFile("inlinepgpgencrypted-error.mbox"_L1));
        otp.print();
        otp.decryptAndVerify();
        otp.print();
        auto partList = otp.collectContentParts();
        QCOMPARE(partList.size(), 1);
        auto part = partList[0].dynamicCast<MimeTreeParser::EncryptedMessagePart>();
        QVERIFY(bool(part));
        QVERIFY(part->error());
    }

    void testEncapsulated()
    {
        MimeTreeParser::ObjectTreeParser otp;
        otp.parseObjectTree(readMailFromFile("encapsulated-with-attachment.mbox"_L1));
        otp.decryptAndVerify();
        auto partList = otp.collectContentParts();
        QCOMPARE(partList.size(), 2);
        auto part = partList[1].dynamicCast<MimeTreeParser::EncapsulatedRfc822MessagePart>();
        QVERIFY(bool(part));
        QCOMPARE(part->from(), "Thomas McGuire <dontspamme@gmx.net>"_L1);
        QCOMPARE(part->date().toString(), "Wed Aug 5 10:57:58 2009 GMT+0200"_L1);
        auto subPartList = otp.collectContentParts(part);
        QCOMPARE(subPartList.size(), 1);
        qWarning() << subPartList[0]->metaObject()->className();
        auto subPart = subPartList[0].dynamicCast<MimeTreeParser::TextMessagePart>();
        QVERIFY(bool(subPart));
    }

    void test8bitEncodedInPlaintext()
    {
        MimeTreeParser::ObjectTreeParser otp;
        otp.parseObjectTree(readMailFromFile("8bitencoded.mbox"_L1));
        QVERIFY(otp.plainTextContent().contains(QString::fromUtf8("Why Pisa’s Tower")));
        QVERIFY(otp.htmlContent().contains(QString::fromUtf8("Why Pisa’s Tower"_L1)));
    }

    void testInlineSigned()
    {
        MimeTreeParser::ObjectTreeParser otp;
        otp.parseObjectTree(readMailFromFile("openpgp-inline-signed.mbox"_L1));
        otp.decryptAndVerify();
        auto partList = otp.collectContentParts();
        QCOMPARE(partList.size(), 1);
        auto part = partList[0].dynamicCast<MimeTreeParser::MessagePart>();
        QCOMPARE(part->signatures().size(), 1);
        QCOMPARE(part->encryptionState(), MimeTreeParser::KMMsgNotEncrypted);
        QCOMPARE(part->signatureState(), MimeTreeParser::KMMsgFullySigned);
        QCOMPARE(part->text(), QString::fromUtf8("ohno öäü\n"));

        QVERIFY(otp.plainTextContent().contains(QString::fromUtf8("ohno öäü"_L1)));

        const auto details = PartModel::signatureDetails(part.get());
        const QString detailsWithoutTimestamp = QString{details}.replace(QRegularExpression{u"on .* with"_s}, u"on TIMESTAMP with"_s);
        QCOMPARE(detailsWithoutTimestamp,
                 "Signature created on TIMESTAMP with certificate: <a "
                 "href=\"key:1BA323932B3FAA826132C79E8D9860C58F246DE6\">unittest key (no password) &lt;test@kolab.org&gt; "
                 "(8D98 60C5 8F24 6DE6)</a><br/>The signature is valid and the certificate's validity is ultimately trusted."_L1);
    }

    void testEncryptedAndSigned()
    {
        MimeTreeParser::ObjectTreeParser otp;
        otp.parseObjectTree(readMailFromFile("openpgp-encrypted+signed.mbox"_L1));
        otp.decryptAndVerify();
        auto partList = otp.collectContentParts();
        QCOMPARE(partList.size(), 1);
        auto part = partList[0].dynamicCast<MimeTreeParser::MessagePart>();
        QCOMPARE(part->signatures().size(), 1);
        QCOMPARE(part->encryptions().size(), 1);
        QCOMPARE(part->encryptionState(), MimeTreeParser::KMMsgFullyEncrypted);
        QCOMPARE(part->signatureState(), MimeTreeParser::KMMsgFullySigned);
        QVERIFY(otp.plainTextContent().contains("encrypted message text"_L1));

        const auto details = PartModel::signatureDetails(part.get());
        const QString detailsWithoutTimestamp = QString{details}.replace(QRegularExpression{u"on .* with"_s}, u"on TIMESTAMP with"_s);
        QCOMPARE(detailsWithoutTimestamp,
                 "Signature created on TIMESTAMP with certificate: <a "
                 "href=\"key:1BA323932B3FAA826132C79E8D9860C58F246DE6\">unittest key (no password) &lt;test@kolab.org&gt; "
                 "(8D98 60C5 8F24 6DE6)</a><br/>The signature is valid and the certificate's validity is ultimately trusted."_L1);
    }

    void testOpenpgpMultipartEmbedded()
    {
        MimeTreeParser::ObjectTreeParser otp;
        otp.parseObjectTree(readMailFromFile("openpgp-multipart-embedded.mbox"_L1));
        otp.print();
        otp.decryptAndVerify();
        otp.print();
        auto partList = otp.collectContentParts();
        QCOMPARE(partList.size(), 1);
        auto part = partList[0].dynamicCast<MimeTreeParser::MessagePart>();
        QCOMPARE(part->encryptions().size(), 1);
        QCOMPARE(part->encryptionState(), MimeTreeParser::KMMsgFullyEncrypted);
        QCOMPARE(otp.plainTextContent(), "sdflskjsdf\n\n-- \nThis is a HTML signature.\n"_L1);
    }

    void testOpenpgpMultipartEmbeddedSigned()
    {
        MimeTreeParser::ObjectTreeParser otp;
        otp.parseObjectTree(readMailFromFile("openpgp-multipart-embedded-signed.mbox"_L1));
        otp.decryptAndVerify();
        auto partList = otp.collectContentParts();
        QCOMPARE(partList.size(), 1);
        auto part = partList[0].dynamicCast<MimeTreeParser::MessagePart>();
        QCOMPARE(part->encryptions().size(), 1);
        QCOMPARE(part->signatures().size(), 1);
        QCOMPARE(part->encryptionState(), MimeTreeParser::KMMsgFullyEncrypted);
        QCOMPARE(part->signatureState(), MimeTreeParser::KMMsgFullySigned);
        QCOMPARE(otp.plainTextContent(), "test\n\n-- \nThis is a HTML signature.\n"_L1);

        const auto details = PartModel::signatureDetails(part.get());
        const QString detailsWithoutTimestamp = QString{details}.replace(QRegularExpression{u"on .* using"_s}, u"on TIMESTAMP using"_s);
        QCOMPARE(detailsWithoutTimestamp,
                 "Signature created on TIMESTAMP using an unknown certificate "
                 "with fingerprint <br/><a href='certificate:CBD116485DB9560CA3CD91E02E3B7787B1B75920'>CBD1 1648 5DB9 560C A3CD  91E0 2E3B 7787 "
                 "B1B7 5920</a><br/>You can search "
                 "the certificate on a keyserver or import it from a file."_L1);
    }

    void testAppleHtmlWithAttachments()
    {
        MimeTreeParser::ObjectTreeParser otp;
        otp.parseObjectTree(readMailFromFile("applehtmlwithattachments.mbox"_L1));
        otp.decryptAndVerify();
        auto partList = otp.collectContentParts();
        QCOMPARE(partList.size(), 1);
        auto part = partList[0].dynamicCast<MimeTreeParser::AlternativeMessagePart>();
        QVERIFY(part);
        QCOMPARE(part->encryptions().size(), 0);
        QCOMPARE(part->signatures().size(), 0);
        QVERIFY(part->isHtml());
        QCOMPARE(otp.plainTextContent(), QString::fromUtf8("Hi,\n\nThis is an HTML message with attachments.\n\nCheers,\nChristian"));
        QCOMPARE(otp.htmlContent(),
                 QString::fromUtf8(
                     "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=us-ascii\"></head><body style=\"word-wrap: break-word; "
                     "-webkit-nbsp-mode: space; line-break: after-white-space;\" class=\"\"><meta http-equiv=\"Content-Type\" content=\"text/html; "
                     "charset=us-ascii\" class=\"\"><div style=\"word-wrap: break-word; -webkit-nbsp-mode: space; line-break: after-white-space;\" "
                     "class=\"\">Hi,<div class=\"\"><br class=\"\"></div><blockquote style=\"margin: 0 0 0 40px; border: none; padding: 0px;\" class=\"\"><div "
                     "class=\"\">This is an <b class=\"\">HTML</b> message with attachments.</div></blockquote><div class=\"\"><br class=\"\"></div><div "
                     "class=\"\">Cheers,</div><div class=\"\">Christian<img apple-inline=\"yes\" id=\"B9EE68A9-83CA-41CD-A3E4-E5BA301F797A\" class=\"\" "
                     "src=\"cid:F5B62D1D-E4EC-4C59-AA5A-708525C2AC3C\"></div></div></body></html>"));

        auto attachments = otp.collectAttachmentParts();
        QCOMPARE(attachments.size(), 1);
    }

    void testAppleHtmlWithAttachmentsMixed()
    {
        MimeTreeParser::ObjectTreeParser otp;
        otp.parseObjectTree(readMailFromFile("applehtmlwithattachmentsmixed.mbox"_L1));
        otp.decryptAndVerify();
        otp.print();
        auto partList = otp.collectContentParts();
        QCOMPARE(partList.size(), 1);
        auto part = partList[0].dynamicCast<MimeTreeParser::AlternativeMessagePart>();
        QVERIFY(part);
        QCOMPARE(part->encryptions().size(), 0);
        QCOMPARE(part->signatures().size(), 0);
        QVERIFY(part->isHtml());
        QCOMPARE(otp.plainTextContent(), "Hello\n\n\n\nRegards\n\nFsdfsdf"_L1);
        QCOMPARE(otp.htmlContent(),
                 "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=us-ascii\"></head><body style=\"word-wrap: break-word; "
                 "-webkit-nbsp-mode: space; line-break: after-white-space;\" class=\"\"><strike class=\"\">Hello</strike><div class=\"\"><br "
                 "class=\"\"></div><div class=\"\"></div></body></html><html><head><meta http-equiv=\"Content-Type\" content=\"text/html; "
                 "charset=us-ascii\"></head><body style=\"word-wrap: break-word; -webkit-nbsp-mode: space; line-break: after-white-space;\" "
                 "class=\"\"><div class=\"\"></div><div class=\"\"><br class=\"\"></div><div class=\"\"><b class=\"\">Regards</b></div><div class=\"\"><b "
                 "class=\"\"><br class=\"\"></b></div><div class=\"\">Fsdfsdf</div></body></html>"_L1);

        auto attachments = otp.collectAttachmentParts();
        QCOMPARE(attachments.size(), 1);
    }

    void testInvitation()
    {
        MimeTreeParser::ObjectTreeParser otp;
        otp.parseObjectTree(readMailFromFile("invitation.mbox"_L1));
        otp.decryptAndVerify();
        otp.print();
        auto partList = otp.collectContentParts();
        QCOMPARE(partList.size(), 1);
        auto part = partList[0].dynamicCast<MimeTreeParser::AlternativeMessagePart>();
        QVERIFY(part);
        QCOMPARE(part->encryptions().size(), 0);
        QCOMPARE(part->signatures().size(), 0);
        QVERIFY(!part->isHtml());
        QVERIFY(part->availableModes().contains(MimeTreeParser::AlternativeMessagePart::MultipartIcal));

        auto attachments = otp.collectAttachmentParts();
        QCOMPARE(attachments.size(), 0);
    }

    void testGmailInvitation()
    {
        MimeTreeParser::ObjectTreeParser otp;
        otp.parseObjectTree(readMailFromFile("gmail-invitation.mbox"_L1));
        otp.decryptAndVerify();
        otp.print();
        auto partList = otp.collectContentParts();
        QCOMPARE(partList.size(), 1);
        auto part = partList[0].dynamicCast<MimeTreeParser::AlternativeMessagePart>();
        QVERIFY(part);
        QCOMPARE(part->encryptions().size(), 0);
        qWarning() << part;
        QCOMPARE(part->signatures().size(), 0);
        QVERIFY(part->isHtml());
        QVERIFY(part->availableModes().contains(MimeTreeParser::AlternativeMessagePart::MultipartIcal));

        auto attachments = otp.collectAttachmentParts();
        QCOMPARE(attachments.size(), 1);
    }

    void testMemoryHole()
    {
        MimeTreeParser::ObjectTreeParser otp;
        otp.parseObjectTree(readMailFromFile("openpgp-encrypted-memoryhole.mbox"_L1));
        otp.decryptAndVerify();
        otp.print();

        auto partList = otp.collectContentParts();
        QCOMPARE(partList.size(), 1);
        auto part = partList[0].dynamicCast<MimeTreeParser::MessagePart>();
        QVERIFY(bool(part));

        QCOMPARE(part->text(), u"very secret mesage\n"_s);

        QCOMPARE(part->header("subject")->asUnicodeString(), "hidden subject"_L1);
        QCOMPARE(part->header("from")->asUnicodeString(), "you@example.com"_L1);
        QCOMPARE(part->header("to")->asUnicodeString(), "me@example.com"_L1);
        QCOMPARE(part->header("cc")->asUnicodeString(), "cc@example.com"_L1);
        QCOMPARE(part->header("message-id")->asUnicodeString(), "<myhiddenreference@me>"_L1);
        QCOMPARE(part->header("references")->asUnicodeString(), "<hiddenreference@hidden>"_L1);
        QCOMPARE(part->header("in-reply-to")->asUnicodeString(), "<hiddenreference@hidden>"_L1);
        QCOMPARE(static_cast<const KMime::Headers::Date *>(part->header("date"))->dateTime(), QDateTime(QDate(2018, 1, 2), QTime(3, 4, 5), QTimeZone::utc()));
    }

    /**
     * Required special handling because the list replaces the toplevel part.
     */
    void testMemoryHoleWithList()
    {
        MimeTreeParser::ObjectTreeParser otp;
        otp.parseObjectTree(readMailFromFile("cid-links-forwarded-inline.mbox"_L1));
        const auto parts = otp.collectContentParts();
        auto part = parts[0];
        QVERIFY(part->header("references"));
        QCOMPARE(part->header("references")->asUnicodeString(), "<a1777ec781546ccc5dcd4918a5e4e03d@info>"_L1);
    }

    void testMemoryHoleMultipartMixed()
    {
        MimeTreeParser::ObjectTreeParser otp;
        otp.parseObjectTree(readMailFromFile("openpgp-encrypted-memoryhole2.mbox"_L1));
        otp.decryptAndVerify();
        otp.print();

        auto partList = otp.collectContentParts();
        QCOMPARE(partList.size(), 1);
        auto part = partList[0].dynamicCast<MimeTreeParser::MessagePart>();
        QVERIFY(bool(part));

        QCOMPARE(part->text(), "\n\n  Fsdflkjdslfj\n\n\nHappy Monday!\n\nBelow you will find a quick overview of the current on-goings. Remember\n"_L1);

        QCOMPARE(part->header("subject")->asUnicodeString(), "This is the subject"_L1);
    }

    void testMIMESignature()
    {
        MimeTreeParser::ObjectTreeParser otp;
        otp.parseObjectTree(readMailFromFile("text+html-maillinglist.mbox"_L1));
        otp.decryptAndVerify();
        otp.print();

        auto partList = otp.collectContentParts();
        for (const auto &part : partList) {
            qWarning() << "found part " << part->metaObject()->className();
        }
        QCOMPARE(partList.size(), 2);
        // The actual content
        {
            auto part = partList[0].dynamicCast<MimeTreeParser::AlternativeMessagePart>();
            QVERIFY(bool(part));
        }

        // The signature
        {
            auto part = partList[1].dynamicCast<MimeTreeParser::TextMessagePart>();
            QVERIFY(bool(part));
            QVERIFY(part->text().contains("bugzilla mailing list"_L1));
        }
    }

    void testCRLFEncryptedWithSignature()
    {
        MimeTreeParser::ObjectTreeParser otp;
        otp.parseObjectTree(readMailFromFile("crlf-encrypted-with-signature.mbox"_L1));
        otp.decryptAndVerify();
        otp.print();

        QCOMPARE(otp.plainTextContent(), "CRLF file\n\n-- \nThis is a signature\nWith two lines\n\nAand another line\n"_L1);
    }

    void testCRLFEncryptedWithSignatureMultipart()
    {
        MimeTreeParser::ObjectTreeParser otp;
        otp.parseObjectTree(readMailFromFile("crlf-encrypted-with-signature-multipart.mbox"_L1));
        otp.decryptAndVerify();
        otp.print();

        // QEXPECT_FAIL("", "because MessagePart::parseInternal uses \n\n to detect encapsulated messages (so 'CRLF file' ends up as header)", Continue);
        // QCOMPARE(otp.plainTextContent(), u"CRLF file\n\n-- \nThis is a signature\nWith two lines\n\nAand another line\n"_s);
        // QVERIFY(!otp.htmlContent().contains("\r\n"_L1));
    }

    void testCRLFOutlook()
    {
        MimeTreeParser::ObjectTreeParser otp;
        otp.parseObjectTree(readMailFromFile("outlook.mbox"_L1));
        otp.decryptAndVerify();
        otp.print();

        qWarning() << otp.plainTextContent();
        QVERIFY(otp.plainTextContent().startsWith(u"Hi Christian,\n\nhabs gerade getestet:\n\n«This is a test"_s));
        QVERIFY(!otp.htmlContent().contains("\r\n"_L1));
    }

    void testOpenPGPEncryptedSignedThunderbird()
    {
        MimeTreeParser::ObjectTreeParser otp;
        otp.parseObjectTree(readMailFromFile("openpgp-encrypted-signed-thunderbird.mbox"_L1));
        otp.print();
        otp.decryptAndVerify();
        otp.print();
        auto partList = otp.collectContentParts();
        QCOMPARE(partList.size(), 1);
        auto part = partList[0].dynamicCast<MimeTreeParser::MessagePart>();
        QVERIFY(bool(part));
        QCOMPARE(part->text(), "sdfsdf\n"_L1);
        QCOMPARE(part->charset(), u"utf-8"_s.toLocal8Bit());
        QCOMPARE(part->encryptions().size(), 1);
        QCOMPARE(part->signatures().size(), 1);
        QCOMPARE(PartModel::signatureSecurityLevel(part.get()), PartModel::Good);
        QCOMPARE(part->encryptionState(), MimeTreeParser::KMMsgFullyEncrypted);
        QCOMPARE(part->signatureState(), MimeTreeParser::KMMsgFullySigned);
        auto contentAttachmentList = otp.collectAttachmentParts();
        QCOMPARE(contentAttachmentList.size(), 1);
        // QCOMPARE(contentAttachmentList[0]->content().size(), 1);
        QCOMPARE(contentAttachmentList[0]->encryptions().size(), 1);
        QCOMPARE(contentAttachmentList[0]->signatures().size(), 1);
        QCOMPARE(contentAttachmentList[0]->encryptionState(), MimeTreeParser::KMMsgFullyEncrypted);
        QCOMPARE(contentAttachmentList[0]->signatureState(), MimeTreeParser::KMMsgFullySigned);
    }

    void testSignedForwardOpenpgpSignedEncrypted()
    {
        MimeTreeParser::ObjectTreeParser otp;
        otp.parseObjectTree(readMailFromFile("signed-forward-openpgp-signed-encrypted.mbox"_L1));
        otp.print();
        otp.decryptAndVerify();
        otp.print();
        auto partList = otp.collectContentParts();
        QCOMPARE(partList.size(), 2);
        auto part = partList[0].dynamicCast<MimeTreeParser::MessagePart>();
        QVERIFY(bool(part));
        QCOMPARE(part->text(), "bla bla bla"_L1);

        part = partList[1].dynamicCast<MimeTreeParser::MessagePart>();
        QVERIFY(bool(part));
        QCOMPARE(part->text(), QString());
        QCOMPARE(part->charset(), u"UTF-8"_s.toLocal8Bit());
        QCOMPARE(part->signatures().size(), 1);
        QCOMPARE(PartModel::signatureSecurityLevel(part.get()), PartModel::Good);
        auto contentAttachmentList = otp.collectAttachmentParts();
        QCOMPARE(contentAttachmentList.size(), 1);
    }

    void testSmimeOpaqueSign()
    {
        MimeTreeParser::ObjectTreeParser otp;
        otp.parseObjectTree(readMailFromFile("smime-opaque-sign.mbox"_L1));
        otp.print();
        otp.decryptAndVerify();
        otp.print();
        auto partList = otp.collectContentParts();
        QCOMPARE(partList.size(), 1);
        auto part = partList[0].dynamicCast<MimeTreeParser::MessagePart>();
        QVERIFY(bool(part));
        QCOMPARE(part->text(), "A simple signed only test."_L1);
    }

    void testSmimeEncrypted()
    {
        MimeTreeParser::ObjectTreeParser otp;
        otp.parseObjectTree(readMailFromFile("smime-encrypted.mbox"_L1));
        otp.print();
        otp.decryptAndVerify();
        otp.print();
        auto partList = otp.collectContentParts();
        QCOMPARE(partList.size(), 1);
        auto part = partList[0].dynamicCast<MimeTreeParser::MessagePart>();
        QVERIFY(bool(part));
        QCOMPARE(part->text(), "The quick brown fox jumped over the lazy dog."_L1);
    }

    void testSmimeSignedApple()
    {
        MimeTreeParser::ObjectTreeParser otp;
        otp.parseObjectTree(readMailFromFile("smime-signed-apple.mbox"_L1));
        otp.print();
        otp.decryptAndVerify();
        otp.print();
        auto partList = otp.collectContentParts();
        QCOMPARE(partList.size(), 1);
        auto part = partList[0].dynamicCast<MimeTreeParser::MessagePart>();
        QVERIFY(bool(part));
        // QCOMPARE(part->text(), u"A simple signed only test."_s);
    }

    void testSmimeEncryptedOctetStream()
    {
        MimeTreeParser::ObjectTreeParser otp;
        otp.parseObjectTree(readMailFromFile("smime-encrypted-octet-stream.mbox"_L1));
        otp.print();
        otp.decryptAndVerify();
        otp.print();
        auto partList = otp.collectContentParts();
        QCOMPARE(partList.size(), 1);
        auto part = partList[0].dynamicCast<MimeTreeParser::MessagePart>();
        QVERIFY(bool(part));
        QCOMPARE(part->text(), "The quick brown fox jumped over the lazy dog."_L1);
    }

    void testSmimeOpaqueSignedEncryptedAttachment()
    {
        MimeTreeParser::ObjectTreeParser otp;
        otp.parseObjectTree(readMailFromFile("smime-opaque-signed-encrypted-attachment.mbox"_L1));
        otp.print();
        QVERIFY(otp.hasEncryptedParts());
        otp.decryptAndVerify();
        otp.print();
        auto partList = otp.collectContentParts();
        QCOMPARE(partList.size(), 1);
        auto part = partList[0].dynamicCast<MimeTreeParser::MessagePart>();
        QVERIFY(bool(part));
        QCOMPARE(part->text(), "This is an Opaque S/MIME encrypted and signed message with attachment"_L1);
    }

    void testSmimeOpaqueEncSign()
    {
        MimeTreeParser::ObjectTreeParser otp;
        otp.parseObjectTree(readMailFromFile("smime-opaque-enc+sign.mbox"_L1));
        otp.print();
        QVERIFY(otp.hasEncryptedParts());
        QVERIFY(!otp.hasSignedParts());
        otp.decryptAndVerify();
        QVERIFY(otp.hasSignedParts());
        otp.print();
        auto partList = otp.collectContentParts();
        QCOMPARE(partList.size(), 1);
        auto part = partList[0].dynamicCast<MimeTreeParser::MessagePart>();
        QVERIFY(bool(part));
        QCOMPARE(part->text(), "Encrypted and signed mail."_L1);
    }
};

QTEST_GUILESS_MAIN(MimeTreeParserTest)
#include "mimetreeparsertest.moc"

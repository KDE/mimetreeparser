// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "../../src/widgets/messagecontainerwidget_p.h"
#include <MimeTreeParserCore/FileOpener>
#include <MimeTreeParserCore/PartModel>
#include <MimeTreeParserWidgets/MessageViewer>
#include <QLabel>
#include <QPushButton>
#include <QTest>
#include <QVBoxLayout>

using namespace MimeTreeParser::Widgets;
using namespace Qt::Literals::StringLiterals;

class MessageViewerTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:

    void messageViewerSMimeEncrypted()
    {
        auto messages =
            MimeTreeParser::Core::FileOpener::openFile(QLatin1StringView(MAIL_DATA_DIR) + QLatin1Char('/') + QLatin1StringView("smime-encrypted.mbox"));
        QCOMPARE(messages.count(), 1);
        MessageViewer viewer;
        viewer.setMessage(messages[0]);

        auto layout = viewer.findChild<QVBoxLayout *>(QStringLiteral("PartLayout"));
        QVERIFY(layout);

        QCOMPARE(layout->count(), 2);
        auto container = qobject_cast<QWidget *>(layout->itemAt(0)->widget());
        QVERIFY(container);

        auto encryptionMessage = container->findChild<QWidget *>(QStringLiteral("EncryptionMessage"));
        auto summaryLabel = encryptionMessage->findChild<QLabel *>(QStringLiteral("SummaryLabel"));
        QVERIFY(summaryLabel);
        auto detailsLabel = encryptionMessage->findChild<QLabel *>(QStringLiteral("DetailsLabel"));
        QVERIFY(detailsLabel);
        auto detailsButton = encryptionMessage->findChild<QPushButton *>(QStringLiteral("DetailsButton"));
        QVERIFY(detailsButton);
        QCOMPARE(encryptionMessage->accessibleName(), QStringLiteral("Crypto summary"));
        QCOMPARE(summaryLabel->text(), QStringLiteral("This message is encrypted."));
        QCOMPARE(detailsLabel->text(),
                 QStringLiteral("<p>The message is encrypted for the following recipient:<ul><li>unittest cert - KDAB (<a "
                                "href=\"messageviewer:showCertificate#gpgsm ### SMIME ### 4CC658E3212B49DC\">4CC6 58E3 212B 49DC</a>)</li></ul></p>"));
        QCOMPARE(detailsLabel->isVisibleTo(&viewer), false);
        QCOMPARE(detailsButton->isVisibleTo(&viewer), true);
        Q_EMIT detailsButton->clicked();
        QCOMPARE(detailsLabel->isVisibleTo(&viewer), true);

        auto signatureMessage = container->findChild<QWidget *>(QStringLiteral("SignatureMessage"));
        QVERIFY(!signatureMessage);
    }

    void testMixedSignedAndUnsignedParts()
    {
        // The test mail here is clearly unusual, but legal, and could be fabricated by
        // some malicious or non-malicious party. We must not show misleading info esp. on what
        // is covered by signatures and what is not.
        auto messages = MimeTreeParser::Core::FileOpener::openFile(QLatin1StringView(MAIL_DATA_DIR) + QLatin1Char('/')
                                                                   + "openpgp-signed-and-unsigned-attachments-and-parts.mbox"_L1);
        QCOMPARE(messages.count(), 1);
        MessageViewer viewer;
        viewer.setMessage(messages[0]);

        auto signatureBoxes = viewer.findChildren<QWidget *>("SignatureMessage");
        QVERIFY(signatureBoxes.size() >= 1);
        // Logically, there is only one signature, but since it has two inline children, we currently show two signature headers.
        // That is something to change, sooner or later, but not the main point of this test.
        // QCOMPARE(signatureBoxes.size(), 1);

        auto attachmentBoxes = viewer.findChildren<QWidget *>("AttachmentBox");
        // exactly one inside, and one outside the signature
        QCOMPARE(attachmentBoxes.size(), 2);
        QVERIFY(attachmentBoxes.value(0)->parent() == signatureBoxes.last()->parent());
        QVERIFY(attachmentBoxes.value(1)->parent() != signatureBoxes.last()->parent());

        // each attachment box shall hold exactly one attachment
        QCOMPARE(attachmentBoxes.value(0)->findChildren<QWidget *>(Qt::FindDirectChildrenOnly).size(), 1);
        QCOMPARE(attachmentBoxes.value(1)->findChildren<QWidget *>(Qt::FindDirectChildrenOnly).size(), 1);
    }
};

QTEST_MAIN(MessageViewerTest)
#include "messageviewertest.moc"

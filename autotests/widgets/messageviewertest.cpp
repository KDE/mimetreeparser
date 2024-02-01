// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "../../src/widgets/messagecontainerwidget_p.h"
#include <KMessageWidget>
#include <MimeTreeParserCore/FileOpener>
#include <MimeTreeParserWidgets/MessageViewer>
#include <QTest>
#include <QVBoxLayout>

using namespace MimeTreeParser::Widgets;

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
        auto container = qobject_cast<MessageWidgetContainer *>(layout->itemAt(0)->widget());
        QVERIFY(container);

        auto encryptionMessage = container->findChild<KMessageWidget *>(QStringLiteral("EncryptionMessage"));
        QCOMPARE(encryptionMessage->messageType(), KMessageWidget::Positive);
        QCOMPARE(encryptionMessage->text(), QStringLiteral("This message is encrypted. <a href=\"messageviewer:showDetails\">Details</a>"));

        encryptionMessage->linkActivated(QStringLiteral("messageviewer:showDetails"));

        QCOMPARE(encryptionMessage->text(),
                 QStringLiteral("This message is encrypted. The message is encrypted for the following keys:<ul><li>unittest cert - KDAB (<a "
                                "href=\"messageviewer:showCertificate#gpgsm ### SMIME ### 4CC658E3212B49DC\">4CC6 58E3 212B 49DC</a>)</li></ul>"));

        auto signatureMessage = container->findChild<KMessageWidget *>(QStringLiteral("SignatureMessage"));
        QVERIFY(!signatureMessage);
    }
};

QTEST_MAIN(MessageViewerTest)
#include "messageviewertest.moc"

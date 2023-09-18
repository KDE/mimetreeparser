// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "../messagecontainerwidget_p.h"
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
    void messageViewerPgpInline()
    {
        auto messages = MimeTreeParser::Core::FileOpener::openFile(QLatin1String(MAIL_DATA_DIR) + QLatin1Char('/')
                                                                   + QLatin1String("openpgp-encrypted-attachment-and-non-encrypted-attachment.mbox"));
        QCOMPARE(messages.count(), 1);
        MessageViewer viewer;
        viewer.setMessage(messages[0]);

        auto layout = viewer.findChild<QVBoxLayout *>(QStringLiteral("PartLayout"));
        QVERIFY(layout);

        QCOMPARE(layout->count(), 2);
        qWarning() << qobject_cast<KMessageWidget *>(layout->itemAt(0)->widget())->text();
        auto container = qobject_cast<MessageWidgetContainer *>(layout->itemAt(0)->widget());
        QVERIFY(container);
    }
};

QTEST_MAIN(MessageViewerTest)
#include "messageviewertest.moc"

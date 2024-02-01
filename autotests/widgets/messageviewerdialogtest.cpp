// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include <MimeTreeParserWidgets/MessageViewerDialog>
#include <QLayout>
#include <QTest>

using namespace MimeTreeParser::Widgets;

class MessageViewerDialogTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void messageViewerDialogCreationMultipleTest()
    {
        MessageViewerDialog dialog(QLatin1StringView(MAIL_DATA_DIR) + QLatin1Char('/') + QLatin1StringView("combined.mbox"));
        QCOMPARE(dialog.messages().count(), 3);

        QCOMPARE(dialog.layout()->count(), 2);
        QVERIFY(dialog.layout()->itemAt(1)->layout());
        QCOMPARE(dialog.layout()->itemAt(1)->layout()->count(), 2);

        const auto actions = dialog.layout()->menuBar()->actions();
        QCOMPARE(actions.count(), 2);
    }

    void messageViewerDialogCreationTest()
    {
        MessageViewerDialog dialog(QLatin1StringView(MAIL_DATA_DIR) + QLatin1Char('/') + QLatin1StringView("plaintext.mbox"));
        QCOMPARE(dialog.messages().count(), 1);

        QCOMPARE(dialog.layout()->count(), 2);

        QVERIFY(!dialog.layout()->itemAt(0)->widget()->isVisible());

        QVERIFY(dialog.layout()->itemAt(1)->layout());
        QCOMPARE(dialog.layout()->itemAt(1)->layout()->count(), 2);
    }
};

QTEST_MAIN(MessageViewerDialogTest)
#include "messageviewerdialogtest.moc"

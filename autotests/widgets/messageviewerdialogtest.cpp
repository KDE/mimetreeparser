// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include <MimeTreeParserWidgets/MessageViewerDialog>

#include <QDialogButtonBox>
#include <QLayout>
#include <QMenu>
#include <QStackedWidget>
#include <QTest>
#include <QToolBar>

using namespace MimeTreeParser::Widgets;

class MessageViewerDialogTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void messageViewerDialogCreationMultipleTest()
    {
        MessageViewerDialog dialog(QLatin1StringView(MAIL_DATA_DIR) + QLatin1Char('/') + QLatin1StringView("combined.mbox"));
        QCOMPARE(dialog.messages().count(), 3);

        QCOMPARE(dialog.layout()->count(), 3); // toolbar, central widget, button box

        const auto toolBar = qobject_cast<QToolBar *>(dialog.layout()->itemAt(0)->widget());
        QVERIFY(toolBar);
        QVERIFY(!toolBar->isHidden());

        const auto centralWidget = qobject_cast<QStackedWidget *>(dialog.layout()->itemAt(1)->widget());
        QVERIFY(centralWidget);
        QCOMPARE(centralWidget->currentIndex(), 0);

        const auto buttonBox = qobject_cast<QDialogButtonBox *>(dialog.layout()->itemAt(2)->widget());
        QVERIFY(buttonBox);

        const auto menuActions = dialog.layout()->menuBar()->actions();
        QCOMPARE(menuActions.count(), 3); // File, View, Navigation
        const auto fileActions = menuActions[0]->menu()->actions();
        QCOMPARE(fileActions.size(), 4);
        QVERIFY(std::ranges::all_of(fileActions, std::mem_fn(&QAction::isEnabled)));
        const auto viewActions = menuActions[1]->menu()->actions();
        QCOMPARE(viewActions.size(), 2);
        QVERIFY(std::ranges::all_of(viewActions, std::mem_fn(&QAction::isEnabled)));
        const auto navigationActions = menuActions[2]->menu()->actions();
        QCOMPARE(navigationActions.size(), 2);
        QVERIFY(!navigationActions[0]->isEnabled());
        QVERIFY(navigationActions[1]->isEnabled());
    }

    void messageViewerDialogCreationTest()
    {
        MessageViewerDialog dialog(QLatin1StringView(MAIL_DATA_DIR) + QLatin1Char('/') + QLatin1StringView("plaintext.mbox"));
        QCOMPARE(dialog.messages().count(), 1);

        QCOMPARE(dialog.layout()->count(), 3); // toolbar, central widget, button box

        const auto toolBar = qobject_cast<QToolBar *>(dialog.layout()->itemAt(0)->widget());
        QVERIFY(toolBar);
        QVERIFY(toolBar->isHidden());

        const auto centralWidget = qobject_cast<QStackedWidget *>(dialog.layout()->itemAt(1)->widget());
        QVERIFY(centralWidget);
        QCOMPARE(centralWidget->currentIndex(), 0);

        const auto buttonBox = qobject_cast<QDialogButtonBox *>(dialog.layout()->itemAt(2)->widget());
        QVERIFY(buttonBox);

        const auto menuActions = dialog.layout()->menuBar()->actions();
        QCOMPARE(menuActions.count(), 3); // File, View, Navigation
        const auto fileActions = menuActions[0]->menu()->actions();
        QCOMPARE(fileActions.size(), 4);
        QVERIFY(std::ranges::all_of(fileActions, std::mem_fn(&QAction::isEnabled)));
        const auto viewActions = menuActions[1]->menu()->actions();
        QCOMPARE(viewActions.size(), 2);
        QVERIFY(std::ranges::all_of(viewActions, std::mem_fn(&QAction::isEnabled)));
    }

    void messageViewerDialogCreationNoMessagesTest()
    {
        MessageViewerDialog dialog(QLatin1StringView(MAIL_DATA_DIR) + QLatin1Char('/') + QLatin1StringView("empty.mbox"));
        QCOMPARE(dialog.messages().count(), 0);

        QCOMPARE(dialog.layout()->count(), 3); // toolbar, central widget, button box

        const auto toolBar = qobject_cast<QToolBar *>(dialog.layout()->itemAt(0)->widget());
        QVERIFY(toolBar);
        QVERIFY(toolBar->isHidden());

        const auto centralWidget = qobject_cast<QStackedWidget *>(dialog.layout()->itemAt(1)->widget());
        QVERIFY(centralWidget);
        QCOMPARE(centralWidget->currentIndex(), 1);

        const auto buttonBox = qobject_cast<QDialogButtonBox *>(dialog.layout()->itemAt(2)->widget());
        QVERIFY(buttonBox);

        const auto menuActions = dialog.layout()->menuBar()->actions();
        QCOMPARE(menuActions.count(), 3); // File, View, Navigation
        const auto fileActions = menuActions[0]->menu()->actions();
        QCOMPARE(fileActions.size(), 4);
        QVERIFY(std::ranges::none_of(fileActions, std::mem_fn(&QAction::isEnabled)));
        const auto viewActions = menuActions[1]->menu()->actions();
        QCOMPARE(viewActions.size(), 2);
        QVERIFY(std::ranges::none_of(viewActions, std::mem_fn(&QAction::isEnabled)));
    }
};

QTEST_MAIN(MessageViewerDialogTest)
#include "messageviewerdialogtest.moc"

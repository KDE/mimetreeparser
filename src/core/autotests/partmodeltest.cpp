// SPDX-FileCopyrightText: 2017 Christian Mollekopf <mollekopf@kolabsys.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDebug>
#include <QTest>
#include <QTextDocument>

#include "partmodel.h"

class PartModelTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:

    void initTestCase()
    {
    }

    void testTrim()
    {
        auto result = PartModel::trim(QLatin1String("<p>This is some funky test.</p>\n<p>-- <br>\nChristian Mollekopf<br>\nSenior Software"));
        QCOMPARE(result.second, true);
        QCOMPARE(result.first, QLatin1String("<p>This is some funky test.</p>\n"));
    }

    void testTrimFromPlain()
    {
        // Qt::convertFromPlainText inserts non-breaking spaces
        auto result = PartModel::trim(Qt::convertFromPlainText(QLatin1String("This is some funky text.\n\n-- \nChristian Mollekopf\nSenior Software")));
        QCOMPARE(result.second, true);
        //\u00A0 is a on-breaking space
        const auto expected = QStringLiteral("<p>This is some funky text.</p>\n").replace(QLatin1Char(' '), QChar(0x00a0));
        QCOMPARE(result.first, expected);
    }
};

QTEST_MAIN(PartModelTest)
#include "partmodeltest.moc"

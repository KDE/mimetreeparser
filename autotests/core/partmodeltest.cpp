// SPDX-FileCopyrightText: 2017 Christian Mollekopf <mollekopf@kolabsys.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <KMime/Message>
#include <QAbstractItemModelTester>
#include <QDebug>
#include <QTest>
#include <QTextDocument>

#include "messageparser.h"
#include "partmodel.h"

static KMime::Message::Ptr readMailFromFile(const QString &mailFile)
{
    QFile file(QLatin1StringView(MAIL_DATA_DIR) + QLatin1Char('/') + mailFile);
    file.open(QIODevice::ReadOnly);
    Q_ASSERT(file.isOpen());
    auto mailData = KMime::CRLFtoLF(file.readAll());
    KMime::Message::Ptr message(new KMime::Message);
    message->setContent(mailData);
    message->parse();
    return message;
}

class PartModelTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:

    void initTestCase()
    {
    }

    void testTrim()
    {
        auto result = PartModel::trim(QLatin1StringView("<p>This is some funky test.</p>\n<p>-- <br>\nChristian Mollekopf<br>\nSenior Software"));
        QCOMPARE(result.second, true);
        QCOMPARE(result.first, QLatin1StringView("<p>This is some funky test.</p>\n"));
    }

    void testTrimFromPlain()
    {
        // Qt::convertFromPlainText inserts non-breaking spaces
        auto result = PartModel::trim(Qt::convertFromPlainText(QLatin1StringView("This is some funky text.\n\n-- \nChristian Mollekopf\nSenior Software")));
        QCOMPARE(result.second, true);
        //\u00A0 is a on-breaking space
        const auto expected = QStringLiteral("<p>This is some funky text.</p>\n").replace(QLatin1Char(' '), QChar(0x00a0));
        QCOMPARE(result.first, expected);
    }

    void testModel()
    {
        MessageParser messageParser;
        messageParser.setMessage(readMailFromFile(QLatin1StringView("html.mbox")));

        QFont font{};
        font.setFamily(QStringLiteral("Noto Sans"));
        qGuiApp->setFont(font);

        auto partModel = messageParser.parts();
        new QAbstractItemModelTester(partModel);
        QCOMPARE(partModel->rowCount(), 1);
        QCOMPARE(partModel->data(partModel->index(0, 0), PartModel::TypeRole).value<PartModel::Types>(), PartModel::Types::Plain);
        QCOMPARE(partModel->data(partModel->index(0, 0), PartModel::IsEmbeddedRole).toBool(), false);
        QCOMPARE(partModel->data(partModel->index(0, 0), PartModel::IsErrorRole).toBool(), false);
        QCOMPARE(
            partModel->data(partModel->index(0, 0), PartModel::ContentRole).toString(),
            QStringLiteral("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" "
                           "\"http://www.w3.org/TR/html4/loose.dtd\">\n<html><head><title></title><style>\nbody {\n  overflow:hidden;\n  font-family: \"Noto "
                           "Sans\" ! important;\n  color: #31363b ! important;\n  background-color: #fcfcfc ! important\n}\nblockquote { \n  border-left: 2px "
                           "solid #bdc3c7 ! important;\n}\n</style></head>\n<body>\n<html><body><p><span>HTML</span> text</p></body></html></body></html>"));
    }
};

QTEST_MAIN(PartModelTest)
#include "partmodeltest.moc"

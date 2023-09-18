// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include <QTest>

#include "attachmentmodel.h"
#include "messageparser.h"

#include <QAbstractItemModelTester>
#include <QSignalSpy>
#include <QTemporaryFile>

KMime::Message::Ptr readMailFromFile(const QString &mailFile)
{
    QFile file(QLatin1String(MAIL_DATA_DIR) + QLatin1Char('/') + mailFile);
    file.open(QIODevice::ReadOnly);
    Q_ASSERT(file.isOpen());
    auto mailData = KMime::CRLFtoLF(file.readAll());
    KMime::Message::Ptr message(new KMime::Message);
    message->setContent(mailData);
    message->parse();
    return message;
}

class AttachmentModelTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:

    void openMailWithOneAttachementTest()
    {
        MessageParser messageParser;
        messageParser.setMessage(readMailFromFile(QLatin1String("attachment.mbox")));

        auto attachmentModel = messageParser.attachments();
        new QAbstractItemModelTester(attachmentModel);

        QCOMPARE(attachmentModel->rowCount(), 1);
        QCOMPARE(attachmentModel->data(attachmentModel->index(0, 0), AttachmentModel::TypeRole).toString(), QStringLiteral("image/jpeg"));
        QCOMPARE(attachmentModel->data(attachmentModel->index(0, 0), AttachmentModel::NameRole).toString(), QStringLiteral("aqnaozisxya.jpeg"));
        QCOMPARE(attachmentModel->data(attachmentModel->index(0, 0), AttachmentModel::SizeRole).toString(), QStringLiteral("100.22 KB"));
        QCOMPARE(attachmentModel->data(attachmentModel->index(0, 0), AttachmentModel::IsEncryptedRole).toBool(), false);
        QCOMPARE(attachmentModel->data(attachmentModel->index(0, 0), AttachmentModel::IsSignedRole).toBool(), false);
        QCOMPARE(attachmentModel->data(attachmentModel->index(0, AttachmentModel::IsEncryptedColumn), Qt::CheckStateRole).value<Qt::CheckState>(),
                 Qt::Unchecked);
        QCOMPARE(attachmentModel->data(attachmentModel->index(0, AttachmentModel::IsSignedColumn), Qt::CheckStateRole).value<Qt::CheckState>(), Qt::Unchecked);
        QCOMPARE(attachmentModel->data(attachmentModel->index(0, AttachmentModel::SizeColumn), Qt::DisplayRole).toString(), QStringLiteral("100.22 KB"));
    }

    void saveTest()
    {
        MessageParser messageParser;
        messageParser.setMessage(readMailFromFile(QLatin1String("attachment.mbox")));

        auto attachmentModel = messageParser.attachments();
        QTemporaryFile file;
        QVERIFY(file.open());
        const auto fileName = attachmentModel->saveAttachmentToPath(0, file.fileName());
        QFile file2(fileName);
        QVERIFY(file2.open(QIODevice::ReadOnly | QIODevice::Text));
        QVERIFY(!file2.readAll().isEmpty());
    }

    void openTest()
    {
        MessageParser messageParser;
        messageParser.setMessage(readMailFromFile(QLatin1String("attachment.mbox")));

        auto attachmentModel = messageParser.attachments();
        QSignalSpy spy(attachmentModel, &AttachmentModel::errorOccurred);
        QVERIFY(spy.isValid());

        attachmentModel->openAttachment(0);

        // Check no error occurred
        QCOMPARE(spy.count(), 0);
    }

    void saveReadonlyTest()
    {
        MessageParser messageParser;
        messageParser.setMessage(readMailFromFile(QLatin1String("attachment.mbox")));

        auto attachmentModel = messageParser.attachments();
        QTemporaryFile file;
        QVERIFY(file.open());
        const auto fileName = attachmentModel->saveAttachmentToPath(0, file.fileName(), true);
        QFile file2(fileName);
        QVERIFY(file2.open(QIODevice::ReadOnly | QIODevice::Text));
        QVERIFY(!file2.readAll().isEmpty());
        QVERIFY(file.permissions() & QFileDevice::ReadUser);
    }

    void saveInvalidPathTest()
    {
        MessageParser messageParser;
        messageParser.setMessage(readMailFromFile(QLatin1String("attachment.mbox")));

        auto attachmentModel = messageParser.attachments();
        QSignalSpy spy(attachmentModel, &AttachmentModel::errorOccurred);
        QVERIFY(spy.isValid());

        const auto fileName = attachmentModel->saveAttachmentToPath(0, QStringLiteral("/does/not/exist"));
        QList<QVariant> arguments = spy.takeFirst();
        QVERIFY(arguments.at(0).userType() == QMetaType::QString);
    }
};

QTEST_MAIN(AttachmentModelTest)
#include "attachmentmodeltest.moc"

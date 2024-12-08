// SPDX-FileCopyrightText: 2023 Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include <KLocalizedContext>
#include <KLocalizedString>
#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QDir>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>

#include "messagehandler.h"
#include <KLocalizedContext>
#include <KLocalizedQmlContext>
#include <MimeTreeParserCore/FileOpener>
#include <MimeTreeParserCore/MessageParser>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription(i18n("View mbox file"));
    parser.addPositionalArgument(QStringLiteral("file"), i18n("mbox file"));

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextObject(new KLocalizedQmlContext(&engine));

    constexpr auto uri = "org.kde.mimetreeparser";

    qmlRegisterType<MessageParser>(uri, 1, 0, "MessageParser");
    qmlRegisterType<MessageHandler>(uri, 1, 0, "MessageHandler");

    engine.load(QUrl(QStringLiteral("qrc:/content/main.qml")));
    const auto rootObjects = engine.rootObjects();
    if (rootObjects.isEmpty()) {
        qWarning() << "Impossible to load main.qml. Please verify installation";
        return -1;
    }
    parser.process(app);

    const QStringList args = parser.positionalArguments();
    if (!args.isEmpty()) {
        for (auto obj : rootObjects) {
            auto view = qobject_cast<QQuickWindow *>(obj);
            auto messageHandler = view->findChild<MessageHandler *>(QStringLiteral("MessageHandler"));
            const auto file = QUrl::fromUserInput(args.at(args.count() - 1), QDir::currentPath());
            messageHandler->open(file);
        }
    }

    return QCoreApplication::exec();
}

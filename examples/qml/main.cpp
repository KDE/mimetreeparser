// SPDX-FileCopyrightText: 2023 Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include <KLocalizedContext>
#include <KLocalizedString>
#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>

#include "fileopener.h"
#include <MimeTreeParser/MessageParser>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription(i18n("View mbox file"));
    parser.addPositionalArgument(QStringLiteral("file"), i18n("mbox file"));

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    QObject::connect(&engine, &QQmlApplicationEngine::quit, &app, &QCoreApplication::quit);

    constexpr auto uri = "org.kde.mimetreeparser";

    qmlRegisterType<FileOpener>(uri, 1, 0, "FileOpener");
    qmlRegisterType<MessageParser>(uri, 1, 0, "MessageParser");

    engine.load(QUrl(QStringLiteral("qrc:/content/main.qml")));
    const auto rootObjects = engine.rootObjects();
    if (rootObjects.isEmpty()) {
        return -1;
    }
    parser.process(app);

    const QStringList args = parser.positionalArguments();
    if (args.length() > 0) {
        for (auto obj : rootObjects) {
            auto view = qobject_cast<QQuickWindow *>(obj);
            auto fileOpener = view->findChild<FileOpener *>(QStringLiteral("FileOpener"));
            const auto file = QUrl::fromUserInput(args.at(args.count() - 1), QDir::currentPath());
            fileOpener->open(file);
        }
    }

    return QCoreApplication::exec();
}

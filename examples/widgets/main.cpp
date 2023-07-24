// SPDX-FileCopyrightText: 2023 Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "../qml/fileopener.h"
#include <KLocalizedString>
#include <MimeTreeParserWidgets/MessageViewer>
#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QMainWindow>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    KLocalizedString::setApplicationDomain("mimetreeparser");
    QCommandLineParser parser;
    parser.setApplicationDescription(i18n("View mbox file"));
    parser.addPositionalArgument(QStringLiteral("file"), i18n("mbox file"));

    parser.process(app);
    const QStringList args = parser.positionalArguments();

    auto messageViewer = new MimeTreeParser::Widgets::MessageViewer;

    QMainWindow mainWindow;
    mainWindow.setMinimumSize(800, 600);
    mainWindow.setCentralWidget(messageViewer);
    mainWindow.show();

    FileOpener fileOpener;

    const auto file = QUrl::fromUserInput(args.at(args.count() - 1), QDir::currentPath());
    QObject::connect(&fileOpener, &FileOpener::messageOpened, messageViewer, [messageViewer](KMime::Message::Ptr message) {
        messageViewer->setMessage(message);
    });
    fileOpener.open(file);

    return app.exec();
}

// SPDX-FileCopyrightText: 2023 Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include <KLocalizedString>
#include <MimeTreeParserWidgets/MessageViewer>
#include <MimeTreeParserWidgets/MessageViewerDialog>
#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QUrl>

#ifdef Q_OS_WIN
#include <cstdio>
#include <windows.h>
#endif

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    KLocalizedString::setApplicationDomain(QByteArrayLiteral("mimetreeparser6"));
#ifdef Q_OS_WIN
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
    }
#endif

    QCommandLineParser parser;
    parser.setApplicationDescription(i18n("View mbox file"));
    parser.addPositionalArgument(QStringLiteral("file"), i18n("mbox file"));

    parser.process(app);
    const QStringList args = parser.positionalArguments();

    const auto file = QUrl::fromUserInput(args.at(args.count() - 1), QDir::currentPath());
    const auto messageViewer = new MimeTreeParser::Widgets::MessageViewerDialog(file.toLocalFile(), nullptr);

    messageViewer->show();
    messageViewer->setAttribute(Qt::WA_DeleteOnClose);
    return app.exec();
}

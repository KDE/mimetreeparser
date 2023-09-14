// SPDX-FileCopyrightText: 2023 Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include <KLocalizedString>
#include <MimeTreeParserWidgets/MessageViewer>
#include <MimeTreeParserWidgets/MessageViewerDialog>
#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QMainWindow>
#include <QUrl>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    KLocalizedString::setApplicationDomain("mimetreeparser");
    QCommandLineParser parser;
    parser.setApplicationDescription(i18n("View mbox file"));
    parser.addPositionalArgument(QStringLiteral("file"), i18n("mbox file"));

    parser.process(app);
    const QStringList args = parser.positionalArguments();

    const auto file = QUrl::fromUserInput(args.at(args.count() - 1), QDir::currentPath());
    const auto messageViewer = new MimeTreeParser::Widgets::MessageViewerDialog(file.toLocalFile(), nullptr);

    return messageViewer->exec();
}

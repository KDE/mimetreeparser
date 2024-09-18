// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "urlhandler.h"

#include "mimetreeparser_core_debug.h"

#include <KLocalizedString>

#include <QCoreApplication>
#include <QProcess>
#include <QStandardPaths>
#include <QUrl>
#include <QWindow>

UrlHandler::UrlHandler(QObject *parent)
    : QObject(parent)
{
}

bool UrlHandler::handleClick(const QUrl &url, QWindow *window)
{
    QStringList lst;
    if (url.scheme() == QStringLiteral("key")) {
        QString keyId = url.path();
        if (keyId.isEmpty()) {
            return false;
        }
        lst << QStringLiteral("--parent-windowid") << QString::number(static_cast<qlonglong>(window->winId())) << QStringLiteral("--query") << keyId;
    } else if (url.scheme() == QStringLiteral("certificate")) {
        QString certificateId = url.path();
        if (certificateId.isEmpty()) {
            return false;
        }
        lst << QStringLiteral("--parent-windowid") << QString::number(static_cast<qlonglong>(window->winId())) << QStringLiteral("--search") << certificateId;
    } else {
        return false;
    }

#ifdef Q_OS_WIN
    QString exec = QStandardPaths::findExecutable(QStringLiteral("kleopatra.exe"), {QCoreApplication::applicationDirPath()});
    if (exec.isEmpty()) {
        exec = QStandardPaths::findExecutable(QStringLiteral("kleopatra.exe"));
    }
#else
    const QString exec = QStandardPaths::findExecutable(QStringLiteral("kleopatra"));
#endif
    if (exec.isEmpty()) {
        qCWarning(MIMETREEPARSER_CORE_LOG) << "Could not find kleopatra executable in PATH";
        Q_EMIT errorOccurred(i18n("Could not start certificate manager. Please check your installation."));
        return false;
    }
    QProcess::startDetached(exec, lst);
    return true;
}

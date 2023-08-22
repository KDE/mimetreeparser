// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "urlhandler_p.h"

#include "mimetreeparser_widgets_debug.h"

#include <KLocalizedString>
#include <KMessageBox>
#include <QProcess>
#include <QStandardPaths>
#include <QUrl>

UrlHandler::UrlHandler(QObject *parent)
    : QObject(parent)
{
}

bool UrlHandler::handleClick(const QUrl &url, QWindow *window)
{
    if (!url.hasFragment()) {
        return false;
    }
    QString displayName;
    QString libName;
    QString keyId;
    if (!foundSMIMEData(url.path() + QLatin1Char('#') + QUrl::fromPercentEncoding(url.fragment().toLatin1()), displayName, libName, keyId)) {
        return false;
    }
    QStringList lst;
    lst << QStringLiteral("--parent-windowid") << QString::number(static_cast<qlonglong>(window->winId())) << QStringLiteral("--query") << keyId;
    const QString exec = QStandardPaths::findExecutable(QStringLiteral("kleopatra"));
    if (exec.isEmpty()) {
        qCWarning(MIMETREEPARSER_WIDGET_LOG) << "Could not find kleopatra executable in PATH";
        KMessageBox::errorWId(window->winId(),
                              i18n("Could not start certificate manager. "
                                   "Please check your installation."),
                              i18n("KMail Error"));
        return false;
    }
    QProcess::startDetached(exec, lst);
    return true;
}

bool UrlHandler::foundSMIMEData(const QString &aUrl, QString &displayName, QString &libName, QString &keyId)
{
    static QString showCertMan(QStringLiteral("showCertificate#"));
    displayName.clear();
    libName.clear();
    keyId.clear();
    int i1 = aUrl.indexOf(showCertMan);
    if (-1 < i1) {
        i1 += showCertMan.length();
        int i2 = aUrl.indexOf(QLatin1String(" ### "), i1);
        if (i1 < i2) {
            displayName = aUrl.mid(i1, i2 - i1);
            i1 = i2 + 5;
            i2 = aUrl.indexOf(QLatin1String(" ### "), i1);
            if (i1 < i2) {
                libName = aUrl.mid(i1, i2 - i1);
                i2 += 5;

                keyId = aUrl.mid(i2);
            }
        }
    }
    return !keyId.isEmpty();
}

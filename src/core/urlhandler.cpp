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

using namespace Qt::StringLiterals;

UrlHandler::UrlHandler(QObject *parent)
    : QObject(parent)
{
}

bool UrlHandler::handleClick(const QUrl &url, QWindow *window)
{
    QString keyId;
    if (url.scheme() == "key"_L1 || url.scheme() == "certificate"_L1) {
        keyId = url.path();
    } else if (!url.hasFragment()) {
        return false;
    }
    QString displayName;
    QString libName;
    if (keyId.isEmpty() && !foundSMIMEData(url.path() + u'#' + QUrl::fromPercentEncoding(url.fragment().toLatin1()), displayName, libName, keyId)) {
        return false;
    }
    const QStringList lst{
        u"--parent-windowid"_s,
        QString::number(static_cast<qlonglong>(window->winId())),
        u"--query"_s,
        keyId,
    };
#ifdef Q_OS_WIN
    QString exec = QStandardPaths::findExecutable(u"kleopatra.exe"_s, {QCoreApplication::applicationDirPath()});
    if (exec.isEmpty()) {
        exec = QStandardPaths::findExecutable(u"kleopatra.exe"_s);
    }
#else
    const QString exec = QStandardPaths::findExecutable(u"kleopatra"_s);
#endif
    if (exec.isEmpty()) {
        qCWarning(MIMETREEPARSER_CORE_LOG) << "Could not find kleopatra executable in PATH";
        Q_EMIT errorOccurred(i18n("Could not start certificate manager. Please check your installation."));
        return false;
    }
    QProcess::startDetached(exec, lst);
    return true;
}

bool UrlHandler::foundSMIMEData(const QString &aUrl, QString &displayName, QString &libName, QString &keyId)
{
    static QString showCertMan(u"showCertificate#"_s);
    displayName.clear();
    libName.clear();
    keyId.clear();
    int i1 = aUrl.indexOf(showCertMan);
    if (-1 < i1) {
        i1 += showCertMan.length();
        int i2 = aUrl.indexOf(" ### "_L1, i1);
        if (i1 < i2) {
            displayName = aUrl.mid(i1, i2 - i1);
            i1 = i2 + 5;
            i2 = aUrl.indexOf(" ### "_L1, i1);
            if (i1 < i2) {
                libName = aUrl.mid(i1, i2 - i1);
                i2 += 5;

                keyId = aUrl.mid(i2);
            }
        }
    }
    return !keyId.isEmpty();
}

#include "moc_urlhandler.cpp"

// SPDX-FileCopyrightText: 2024-2026 Laurent Montel <montel@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "messageviewerutils_p.h"
using namespace Qt::Literals::StringLiterals;

#include <QRegularExpression>

QString MesageViewerUtils::changeExtension(const QString &fileName, const QString &extension)
{
    auto renamedFileName = fileName;
    renamedFileName.replace(QRegularExpression(u"\\.(mbox|p7m|asc)$"_s), extension);

    // In case the file name didn't contain any of the expected extension: mbox, p7m, asc and eml
    // or doesn't contains an extension at all.
    if (!renamedFileName.endsWith(extension)) {
        renamedFileName += extension;
    }

    return renamedFileName;
}

#define SLASHES "/\\"

QString MesageViewerUtils::changeFileName(const QString &fileName, const QString &subject)
{
    if (subject.isEmpty()) {
        return fileName;
    }

    if (fileName.isEmpty()) {
        return subject;
    }

    auto cleanedSubject = subject;

    static const char notAllowedChars[] = ",^@={}[]~!?:&*\"|#%<>$\"'();`'/\\.";
    static const char *notAllowedSubStrings[] = {"..", "  "};

    for (const char *c = notAllowedChars; *c; c++) {
        cleanedSubject.replace(QLatin1Char(*c), u" "_s);
    }

    const int notAllowedSubStringCount = sizeof(notAllowedSubStrings) / sizeof(const char *);
    for (int s = 0; s < notAllowedSubStringCount; s++) {
        const QLatin1StringView notAllowedSubString(notAllowedSubStrings[s]);
        cleanedSubject.replace(notAllowedSubString, u" "_s);
    }

    QStringList splitedFileName = fileName.split(QLatin1Char('/'));
    splitedFileName[splitedFileName.count() - 1] = cleanedSubject;
    return splitedFileName.join(QLatin1Char('/'));
}

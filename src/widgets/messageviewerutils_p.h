// SPDX-FileCopyrightText: 2024-2025 Laurent Montel <montel@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once
#include <QString>

struct MesageViewerUtils {
    static QString changeExtension(const QString &fileName, const QString &extension);
    static QString changeFileName(const QString &fileName, const QString &subject);
};

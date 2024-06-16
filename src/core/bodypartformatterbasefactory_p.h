// SPDX-FileCopyrightText: 2004 Marc Mutz <mutz@kde.org>
// SPDX-FileCopyrightText: 2004 Ingo Kloecker <kloecker@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "bodypartformatterbasefactory.h"
#include <memory>
#include <optional>

namespace MimeTreeParser
{
class BodyPartFormatterBaseFactory;

class BodyPartFormatterBaseFactoryPrivate
{
public:
    BodyPartFormatterBaseFactoryPrivate(BodyPartFormatterBaseFactory *factory);
    ~BodyPartFormatterBaseFactoryPrivate();

    void setup();
    void messageviewer_create_builtin_bodypart_formatters(); // defined in bodypartformatter.cpp
    void insert(const QString &type, const QString &subtype, const Interface::BodyPartFormatter *formatter);
    void loadPlugins();

    BodyPartFormatterBaseFactory *const q;
    std::optional<TypeRegistry> all;
};

}

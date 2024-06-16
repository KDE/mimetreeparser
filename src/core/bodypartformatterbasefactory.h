// SPDX-FileCopyrightText: 2004 Marc Mutz <mutz@kde.org>,
// SPDX-FileCopyrightText: 2004 Ingo Kloecker <kloecker@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QByteArray>
#include <QString>
#include <map>
#include <memory>

namespace MimeTreeParser
{

namespace Interface
{
class BodyPartFormatter;
}

struct ltstr {
    bool operator()(const QString &s1, const QString &s2) const
    {
        return s1.compare(s2);
    }
};

typedef std::multimap<QString, const Interface::BodyPartFormatter *, ltstr> SubtypeRegistry;
typedef std::map<QString, MimeTreeParser::SubtypeRegistry, MimeTreeParser::ltstr> TypeRegistry;

class BodyPartFormatterBaseFactoryPrivate;

class BodyPartFormatterBaseFactory
{
public:
    BodyPartFormatterBaseFactory();
    ~BodyPartFormatterBaseFactory();

    const SubtypeRegistry &subtypeRegistry(const QString &type) const;

protected:
    void insert(const QString &type, const QString &subtype, Interface::BodyPartFormatter *formatter);

private:
    static BodyPartFormatterBaseFactory *mSelf;

    std::unique_ptr<BodyPartFormatterBaseFactoryPrivate> d;
    friend class BodyPartFormatterBaseFactoryPrivate;

private:
    // disabled
    const BodyPartFormatterBaseFactory &operator=(const BodyPartFormatterBaseFactory &);
    BodyPartFormatterBaseFactory(const BodyPartFormatterBaseFactory &);
};

}

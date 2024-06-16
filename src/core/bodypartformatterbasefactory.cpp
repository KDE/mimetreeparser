// SPDX-FileCopyrightText: 2004 Marc Mutz <mutz@kde.org>
// SPDX-FileCopyrightText: 2004 Ingo Kloecker <kloecker@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "bodypartformatterbasefactory.h"
#include "bodypartformatter.h"
#include "bodypartformatterbasefactory_p.h"
#include "mimetreeparser_core_debug.h"

#include <KPluginMetaData>
#include <QJsonArray>
#include <QPluginLoader>

using namespace MimeTreeParser;
using namespace Qt::StringLiterals;

BodyPartFormatterBaseFactoryPrivate::BodyPartFormatterBaseFactoryPrivate(BodyPartFormatterBaseFactory *factory)
    : q(factory)
{
}

BodyPartFormatterBaseFactoryPrivate::~BodyPartFormatterBaseFactoryPrivate()
{
}

void BodyPartFormatterBaseFactoryPrivate::setup()
{
    if (!all) {
        all = std::make_optional<TypeRegistry>();
        // plugins should always be higher than the built-in ones
        loadPlugins();
        messageviewer_create_builtin_bodypart_formatters();
    }
}

void BodyPartFormatterBaseFactoryPrivate::loadPlugins()
{
    const QList<KPluginMetaData> plugins = KPluginMetaData::findPlugins(QStringLiteral("pim6/mimetreeparser/bodypartformatter"));
    for (const auto &md : plugins) {
        const auto formatterData = md.rawData().value(QLatin1StringView("formatter")).toArray();
        if (formatterData.isEmpty()) {
            continue;
        }
        QPluginLoader loader(md.fileName());
        auto plugin = qobject_cast<MimeTreeParser::Interface::BodyPartFormatterPlugin *>(loader.instance());
        if (!plugin) {
            continue;
        }

        for (int i = 0; i < formatterData.size(); ++i) {
            const auto metaData = formatterData.at(i).toObject();
            const auto mimetype = metaData.value(QLatin1StringView("mimetype")).toString();
            if (mimetype.isEmpty()) {
                qCWarning(MIMETREEPARSER_CORE_LOG) << "BodyPartFormatterFactory: plugin" << md.fileName() << "returned empty mimetype specification for index"
                                                   << i;
                break;
            }
            qCDebug(MIMETREEPARSER_CORE_LOG) << "plugin for " << mimetype;
            const auto mimetypeSplit = mimetype.split(u'/');
            if (mimetypeSplit.size() != 2) {
                qCWarning(MIMETREEPARSER_CORE_LOG) << "BodyPartFormatterFactory: plugin" << md.fileName() << "returned invalid mimetype specification for index"
                                                   << i;
                break;
            }

            const MimeTreeParser::Interface::BodyPartFormatter *bfp = plugin->bodyPartFormatter(mimetype);
            if (!bfp) {
                qCWarning(MIMETREEPARSER_CORE_LOG) << "BodyPartFormatterFactory: plugin" << md.fileName() << "returned invalid body part formatter for index"
                                                   << i;
                break;
            }

            insert(mimetypeSplit[0], mimetypeSplit[1], bfp);
        }
    }
}

void BodyPartFormatterBaseFactoryPrivate::insert(const QString &type, const QString &subtype, const Interface::BodyPartFormatter *formatter)
{
    if (type.isEmpty() || subtype.isEmpty() || !formatter || !all) {
        return;
    }

    auto type_it = all->find(type);
    if (type_it == all->end()) {
        type_it = all->insert(std::make_pair(type, SubtypeRegistry())).first;
        Q_ASSERT(type_it != all->end());
    }

    SubtypeRegistry &subtype_reg = type_it->second;

    subtype_reg.insert(std::make_pair(subtype, formatter));
}

BodyPartFormatterBaseFactory::BodyPartFormatterBaseFactory()
    : d(std::make_unique<BodyPartFormatterBaseFactoryPrivate>(this))
{
}

BodyPartFormatterBaseFactory::~BodyPartFormatterBaseFactory() = default;

void BodyPartFormatterBaseFactory::insert(const QString &type, const QString &subtype, Interface::BodyPartFormatter *formatter)
{
    d->insert(type, subtype, formatter);
}

const SubtypeRegistry &BodyPartFormatterBaseFactory::subtypeRegistry(const QString &type) const
{
    const auto nonEmptyType = type.isEmpty() ? u"*"_s : type;

    d->setup();
    Q_ASSERT(d->all);

    static SubtypeRegistry emptyRegistry;
    if (d->all->empty()) {
        return emptyRegistry;
    }

    auto type_it = d->all->find(nonEmptyType);
    if (type_it == d->all->end()) {
        type_it = d->all->find(u"*"_s);
    }
    if (type_it == d->all->end()) {
        return emptyRegistry;
    }

    const SubtypeRegistry &subtype_reg = type_it->second;
    if (subtype_reg.empty()) {
        return emptyRegistry;
    }
    return subtype_reg;
}

/*
   SPDX-FileCopyrightText: 2017 Volker Krause <vkrause@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "itinerarykdeconnecthandler.h"

#ifndef Q_OS_WIN
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusReply>
#endif
#include <QDebug>
#include <QList>
#include <QUrl>

ItineraryKDEConnectHandler::ItineraryKDEConnectHandler(QObject *parent)
    : QObject(parent)
{
#ifndef Q_OS_WIN
    auto msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.kdeconnect"),
                                              QStringLiteral("/modules/kdeconnect"),
                                              QStringLiteral("org.kde.kdeconnect.daemon"),
                                              QStringLiteral("devices"));
    msg.setArguments({true, true});
    QDBusPendingReply<QStringList> reply = QDBusConnection::sessionBus().asyncCall(msg);
    auto watcher = new QDBusPendingCallWatcher(reply, this);
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *call) {
        QDBusPendingReply<QStringList> reply = *call;
        if (reply.isError()) {
            qWarning() << reply.error();
        } else {
            const auto values = reply.value();
            for (const QString &deviceId : values) {
                QDBusInterface deviceIface(QStringLiteral("org.kde.kdeconnect"),
                                           QStringLiteral("/modules/kdeconnect/devices/") + deviceId,
                                           QStringLiteral("org.kde.kdeconnect.device"));
                QDBusReply<bool> pluginReply = deviceIface.call(QStringLiteral("hasPlugin"), QLatin1StringView("kdeconnect_share"));

                if (pluginReply.value()) {
                    m_devices.push_back({deviceId, deviceIface.property("name").toString()});
                }
            }
        }
        Q_EMIT devicesChanged();
        call->deleteLater();
    });
#endif
}

QList<Device> ItineraryKDEConnectHandler::devices() const
{
    return m_devices;
}

void ItineraryKDEConnectHandler::sendToDevice(const QString &fileName, const QString &deviceId)
{
#ifndef Q_OS_WIN
    const QString method = QStringLiteral("openFile");

    QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.kdeconnect"),
                                                      QStringLiteral("/modules/kdeconnect/devices/") + deviceId + QStringLiteral("/share"),
                                                      QStringLiteral("org.kde.kdeconnect.device.share"),
                                                      method);
    msg.setArguments({QUrl::fromLocalFile(fileName).toString()});

    QDBusConnection::sessionBus().send(msg);
#endif
}

#include "moc_itinerarykdeconnecthandler.cpp"

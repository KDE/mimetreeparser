/*
   SPDX-FileCopyrightText: 2017 Volker Krause <vkrause@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>
#include <QString>
#include <qqmlregistration.h>

struct Device {
    Q_GADGET
    QML_ELEMENT
    Q_PROPERTY(QString deviceId MEMBER deviceId)
    Q_PROPERTY(QString name MEMBER name)

public:
    QString deviceId;
    QString name;
};

class ItineraryKDEConnectHandler : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(KDEConnectHandler)
    Q_PROPERTY(QList<Device> devices READ devices NOTIFY devicesChanged)
public:
    explicit ItineraryKDEConnectHandler(QObject *parent = nullptr);

    [[nodiscard]] QList<Device> devices() const;

    Q_INVOKABLE void sendToDevice(const QString &fileName, const QString &deviceId);

private:
    QList<Device> m_devices;

Q_SIGNALS:
    void devicesChanged();
};

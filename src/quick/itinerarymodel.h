// SPDX-FileCopyrightText: 2024 Tobias Fella <tobias.fella@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include <KMime/Message>
#include <QAbstractListModel>
#include <QJsonArray>
#include <QPointer>
#include <QQmlEngine>
#include <QString>

class QTemporaryFile;

class ItineraryModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString path READ path NOTIFY pathChanged)
    Q_PROPERTY(std::shared_ptr<KMime::Message> message READ message WRITE setMessage NOTIFY messageChanged)

public:
    enum Roles {
        NameRole = Qt::DisplayRole,
        TypeRole,
        DepartureLocationRole,
        DepartureTimeRole,
        DepartureDayRole,
        DepartureAddressRole,
        ArrivalLocationRole,
        ArrivalTimeRole,
        ArrivalDayRole,
        ArrivalAddressRole,
        AddressRole,
        StartTimeRole,
        EndTimeRole,
        DeparturePlatformRole,
        ArrivalPlatformRole,
        CoachRole,
        SeatRole,
    };
    Q_ENUM(Roles)
    explicit ItineraryModel(QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &parent = {}) const override;

    QHash<int, QByteArray> roleNames() const override;

    QString path() const;

    std::shared_ptr<KMime::Message> message() const;
    void setMessage(const std::shared_ptr<KMime::Message> &message);

    Q_INVOKABLE void sendToItinerary();

Q_SIGNALS:
    void loaded();
    void loadErrorOccurred();
    void pathChanged();
    void messageChanged();

private:
    QJsonArray m_data;
    QTemporaryFile *m_temporary;
    QString m_path;
    std::shared_ptr<KMime::Message> m_message;
    void loadData();
};

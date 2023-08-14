// SPDX-FileCopyrightText: 2017 Volker Krause <vkrause@kde.org>
// SPDX-FileCopyrightText: 2023 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include <KCalendarCore/Event>

#include <QObject>
#include <QSet>
#include <QVariant>
#include <QVector>

#include <vector>

namespace KPkPass
{
class Pass;
}

namespace KMime
{
class ContentIndex;
}

namespace MimeTreeParser
{
namespace Itinerary
{
/// Memento holding the itinerary information extracted for an email.
class ItineraryMemento : public QObject
{
    Q_OBJECT

public:
    ItineraryMemento();
    ~ItineraryMemento() override;

    Q_REQUIRED_RESULT bool isParsed(const KMime::ContentIndex &index) const;
    void setParsed(const KMime::ContentIndex &index);

    void setMessageDate(const QDateTime &contextDt);
    void appendData(const QVector<QVariant> &data);

    Q_REQUIRED_RESULT bool hasData() const;
    struct TripData {
        QVector<QVariant> reservations;
        KCalendarCore::Event::Ptr event;
        bool expanded;
    };
    Q_REQUIRED_RESULT QVector<TripData> data();
    void toggleExpanded(int index);

    void addPass(KPkPass::Pass *pass, const QByteArray &rawData);
    QByteArray rawPassData(const QString &passTypeIdentifier, const QString &serialNumber) const;

    struct PassData {
        QString passTypeIdentifier;
        QString serialNumber;
        QByteArray rawData;
    };
    const std::vector<PassData> &passData() const;

    struct DocumentData {
        QString docId;
        QVariant docInfo;
        QByteArray rawData;
    };
    const std::vector<DocumentData> &documentData() const;

    void addDocument(const QString &docId, const QVariant &docInfo, const QByteArray &docData);

    /** At least one reservation has enough information to add it to the calendar. */
    bool canAddToCalendar() const;
    /** Start date of the reservation data.
     *  TODO this eventually should include the end date too, for showing the full range in the calendar,
     *  but KOrganizer doesn't support that yet
     */
    QDate startDate() const;

private:
    class Private;
    std::unique_ptr<Private> d;
};
}
}

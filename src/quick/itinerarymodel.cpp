// SPDX-FileCopyrightText: 2024 Tobias Fella <tobias.fella@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "itinerarymodel.h"

#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimeZone>
#include <QUrlQuery>

#include <KCountry>
#include <KFormat>
#include <KLocalizedString>
#include <QProcess>
#include <QTemporaryFile>

#include "config-mimetreeparser-quick.h"

#ifndef Q_OS_ANDROID
#include <KIO/ApplicationLauncherJob>
#endif

using namespace Qt::StringLiterals;

namespace
{
QDateTime reservationDateTime(const QJsonValue &value)
{
    const auto text = (value.isString() ? value : value["@value"_L1]).toString();
    if (text.size() == 10) {
        return QDateTime(QDate::fromString(text, Qt::ISODate), QTime(0, 0));
    }
    auto dateTime = QDateTime::fromString(text, Qt::ISODate);
    const auto zoneId = value["timezone"_L1].toString().toUtf8();
    const QTimeZone zone(zoneId);
    if (!zoneId.isEmpty() && dateTime.isValid() && zone.isValid()) {
        dateTime = dateTime.timeSpec() == Qt::LocalTime ? QDateTime(dateTime.date(), dateTime.time(), zone) : dateTime.toTimeZone(zone);
    }
    return dateTime;
}

QString reservationTime(const QDateTime &dateTime)
{
    return KFormat(QLocale::system()).formatTime(dateTime, QLocale::ShortFormat, KFormat::AddTimezoneAbbreviationIfNeeded);
}

QString lodgingDate(const QJsonValue &value)
{
    const auto dateTime = reservationDateTime(value);
    if (!dateTime.isValid()) {
        return {};
    }
    const auto locale = QLocale::system();
    const auto date = locale.toString(dateTime.date(), QLocale::LongFormat);
    // Extractors represent lodging dates without a known time as midnight.
    if (dateTime.time() == QTime(0, 0)) {
        return date;
    }
    return date + u" · "_s + reservationTime(dateTime);
}

QString readableDateTime(const QJsonValue &value)
{
    const auto dateTime = reservationDateTime(value);
    if (!dateTime.isValid()) {
        return {};
    }
    const auto locale = QLocale::system();
    const auto date = locale.toString(dateTime.date(), QLocale::LongFormat);
    const auto text = (value.isString() ? value : value["@value"_L1]).toString();
    return text.size() == 10 ? date : date + u" · "_s + reservationTime(dateTime);
}

QJsonValue reservationPlace(const QJsonValue &data)
{
    const auto trip = data["reservationFor"_L1];
    return data["@type"_L1].toString() == "EventReservation"_L1 ? trip["location"_L1] : trip;
}

QJsonValue transportStop(const QJsonValue &data, bool departure)
{
    const auto type = data["@type"_L1].toString();
    const auto trip = data["reservationFor"_L1];
    if (type == "FlightReservation"_L1) {
        return trip[departure ? "departureAirport"_L1 : "arrivalAirport"_L1];
    }
    if (type == "TrainReservation"_L1) {
        return trip[departure ? "departureStation"_L1 : "arrivalStation"_L1];
    }
    if (type == "BusReservation"_L1) {
        return trip[departure ? "departureBusStop"_L1 : "arrivalBusStop"_L1];
    }
    if (type == "BoatReservation"_L1) {
        return trip[departure ? "departureBoatTerminal"_L1 : "arrivalBoatTerminal"_L1];
    }
    return {};
}

QString countryName(const QJsonValue &address)
{
    const auto code = address["addressCountry"_L1].toString();
    const auto country = KCountry::fromAlpha2(code);
    return country.isValid() ? country.name() : code;
}

QString joinedNonEmpty(QStringList parts, QStringView separator)
{
    parts.removeAll(QString());
    return parts.join(separator);
}

QString formattedAddress(const QJsonValue &address)
{
    const auto locality =
        joinedNonEmpty({address["postalCode"_L1].toString(), address["addressLocality"_L1].toString(), address["addressRegion"_L1].toString()}, u" ");
    return joinedNonEmpty({address["streetAddress"_L1].toString(), joinedNonEmpty({locality, countryName(address)}, u", ")}, u"\n");
}
}

ItineraryModel::ItineraryModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_temporary(new QTemporaryFile(QDir::tempPath() + QLatin1Char('/') + QCoreApplication::applicationName() + QLatin1String(".XXXXXX.eml"), this))
{
}

QVariant ItineraryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_data.size()) {
        return {};
    }
    const auto data = m_data[index.row()];
    const auto type = data["@type"_L1].toString();
    const auto trip = data["reservationFor"_L1];
    if (role == NameRole) {
        if (type == "FlightReservation"_L1) {
            const auto airline = trip["airline"_L1];
            return joinedNonEmpty({airline["name"_L1].toString(), airline["iataCode"_L1].toString() + trip["flightNumber"_L1].toString()}, u" ");
        }
        if (type == "TrainReservation"_L1) {
            const auto name = joinedNonEmpty({trip["trainName"_L1].toString(), trip["trainNumber"_L1].toString()}, u" ");
            return name.isEmpty() ? i18nc("@title:group", "Train") : name;
        }
        if (type == "BusReservation"_L1) {
            const auto name = joinedNonEmpty({trip["busName"_L1].toString(), trip["busNumber"_L1].toString()}, u" ");
            return name.isEmpty() ? i18nc("@title:group", "Bus") : name;
        }
        if (type == "BoatReservation"_L1) {
            const auto name = trip["name"_L1].toString();
            return name.isEmpty() ? i18nc("@title:group", "Ferry") : name;
        }
        return trip["name"_L1].toString();
    }
    if (role == TypeRole) {
        return type;
    }
    if (role == DepartureLocationRole || role == ArrivalLocationRole) {
        const auto stop = transportStop(data, role == DepartureLocationRole);
        if (type == "FlightReservation"_L1 && !stop["iataCode"_L1].toString().isEmpty()) {
            return stop["iataCode"_L1].toString();
        }
        return stop["name"_L1].toString();
    }
    if (role == DepartureAddressRole || role == ArrivalAddressRole) {
        const auto stop = transportStop(data, role == DepartureAddressRole);
        return type == "FlightReservation"_L1 ? stop["name"_L1].toString() : formattedAddress(stop["address"_L1]);
    }
    if (role == DepartureTimeRole || role == ArrivalTimeRole || role == DepartureDayRole || role == ArrivalDayRole) {
        const bool departure = role == DepartureTimeRole || role == DepartureDayRole;
        auto dateTime = reservationDateTime(trip[departure ? "departureTime"_L1 : "arrivalTime"_L1]);
        if (!dateTime.isValid() && departure && role == DepartureDayRole) {
            dateTime = reservationDateTime(trip["departureDay"_L1]);
        }
        if (!dateTime.isValid()) {
            return QString();
        }
        const auto locale = QLocale::system();
        if (role == DepartureDayRole || role == ArrivalDayRole) {
            return locale.toString(dateTime.date(), QLocale::LongFormat);
        }
        return reservationTime(dateTime);
    }
    if (role == AddressRole) {
        return formattedAddress(reservationPlace(data)["address"_L1]);
    }
    if (role == LocationRole) {
        const auto place = reservationPlace(data);
        if (type == "EventReservation"_L1) {
            return place.isString() ? place.toString() : place["name"_L1].toString();
        }
        const auto address = place["address"_L1];
        return joinedNonEmpty({address["addressLocality"_L1].toString(), countryName(address)}, u", ");
    }
    if (role == NightsRole) {
        const auto checkin = reservationDateTime(data["checkinTime"_L1]).date();
        const auto checkout = reservationDateTime(data["checkoutTime"_L1]).date();
        return checkin.isValid() && checkout.isValid() ? qMax<qint64>(0, checkin.daysTo(checkout)) : 0;
    }
    if (role == MapUrlRole) {
        const auto place = reservationPlace(data);
        const auto geo = place["geo"_L1];
        if (geo["latitude"_L1].isDouble() && geo["longitude"_L1].isDouble()) {
            return QUrl(u"geo:%1,%2"_s.arg(geo["latitude"_L1].toDouble(), 0, 'f', 6).arg(geo["longitude"_L1].toDouble(), 0, 'f', 6));
        }
        const auto address = formattedAddress(place["address"_L1]);
        if (address.isEmpty()) {
            return QUrl();
        }
        QUrl url(u"geo:0,0"_s);
        QUrlQuery query;
        query.addQueryItem(u"q"_s, joinedNonEmpty({place["name"_L1].toString(), address}, u", ").replace(u'\n', ", "_L1));
        url.setQuery(query);
        return url;
    }
    if (role == StartTimeRole) {
        if (type == "LodgingReservation"_L1) {
            return lodgingDate(data["checkinTime"_L1]);
        }
        if (type == "EventReservation"_L1) {
            return readableDateTime(trip["startDate"_L1]);
        }
        if (type == "FlightReservation"_L1) {
            return readableDateTime(trip["boardingTime"_L1]);
        }
        return readableDateTime(data["startTime"_L1]);
    }
    if (role == EndTimeRole) {
        if (type == "LodgingReservation"_L1) {
            return lodgingDate(data["checkoutTime"_L1]);
        }
        return readableDateTime(type == "EventReservation"_L1 ? trip["endDate"_L1] : data["endTime"_L1]);
    }
    if (role == DeparturePlatformRole) {
        return trip["departurePlatform"_L1].toString();
    }
    if (role == ArrivalPlatformRole) {
        return trip["arrivalPlatform"_L1].toString();
    }
    if (role == DepartureTerminalRole) {
        return trip["departureTerminal"_L1].toString();
    }
    if (role == ArrivalTerminalRole) {
        return trip["arrivalTerminal"_L1].toString();
    }
    if (role == CoachRole) {
        return data["reservedTicket"_L1]["ticketedSeat"_L1]["seatSection"_L1].toString();
    }
    if (role == SeatRole) {
        const auto seat = data["airplaneSeat"_L1].toString();
        return seat.isEmpty() ? data["reservedTicket"_L1]["ticketedSeat"_L1]["seatNumber"_L1].toString() : seat;
    }
    return {};
}

int ItineraryModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_data.size();
}

QHash<int, QByteArray> ItineraryModel::roleNames() const
{
    return {
        {NameRole, "name"},
        {TypeRole, "type"},
        {DepartureLocationRole, "departureLocation"},
        {DepartureDayRole, "departureDay"},
        {DepartureAddressRole, "departureAddress"},
        {ArrivalLocationRole, "arrivalLocation"},
        {ArrivalAddressRole, "arrivalAddress"},
        {ArrivalDayRole, "arrivalDay"},
        {DepartureTimeRole, "departureTime"},
        {ArrivalTimeRole, "arrivalTime"},
        {AddressRole, "address"},
        {StartTimeRole, "startTime"},
        {EndTimeRole, "endTime"},
        {DeparturePlatformRole, "departurePlatform"},
        {ArrivalPlatformRole, "arrivalPlatform"},
        {CoachRole, "coach"},
        {SeatRole, "seat"},
        {LocationRole, "location"},
        {NightsRole, "nights"},
        {MapUrlRole, "mapUrl"},
        {DepartureTerminalRole, "departureTerminal"},
        {ArrivalTerminalRole, "arrivalTerminal"},
    };
}

QString ItineraryModel::path() const
{
    return m_path;
}

std::shared_ptr<KMime::Message> ItineraryModel::message() const
{
    return m_message;
}

bool ItineraryModel::saveMessage()
{
    if (!m_message || !m_temporary->open()) {
        return false;
    }
    const auto content = m_message->encodedContent();
    const bool saved = m_temporary->resize(0) && m_temporary->seek(0) && m_temporary->write(content) == content.size() && m_temporary->flush();
    m_temporary->close();
    return saved;
}

void ItineraryModel::setMessage(const std::shared_ptr<KMime::Message> &message)
{
    if (m_message == message) {
        return;
    }
    m_message = message;
    Q_EMIT messageChanged();

    beginResetModel();
    m_data = {};
    endResetModel();
    m_path.clear();
    if (!m_message) {
        Q_EMIT pathChanged();
        return;
    }
    if (!saveMessage()) {
        Q_EMIT pathChanged();
        Q_EMIT loadErrorOccurred();
        return;
    }
    m_path = m_temporary->fileName();
    Q_EMIT pathChanged();
    loadData();
}

void ItineraryModel::loadData()
{
    auto process = new QProcess(this);
    connect(process, &QProcess::finished, this, [this, process, message = m_message]() {
        if (message == m_message) {
            const auto data = process->readAllStandardOutput();
            beginResetModel();
            m_data = QJsonDocument::fromJson(data).array();
            endResetModel();
            Q_EMIT loaded();
        }
        process->deleteLater();
    });
    connect(process, &QProcess::errorOccurred, this, [this, process, message = m_message]() {
        if (message == m_message) {
            Q_EMIT loadErrorOccurred();
        }
        process->deleteLater();
    });
    process->start(QStringLiteral(CMAKE_INSTALL_FULL_LIBEXECDIR_KF6 "/kitinerary-extractor"));
    process->write(m_message->encodedContent());
    process->closeWriteChannel();
}

void ItineraryModel::sendToItinerary()
{
    if (m_path.isEmpty()) {
        return;
    }
    auto job = new KIO::ApplicationLauncherJob(KService::serviceByDesktopName(u"org.kde.itinerary"_s));
    job->setUrls({QUrl::fromLocalFile(m_path)});
    job->start();
}

#include "moc_itinerarymodel.cpp"

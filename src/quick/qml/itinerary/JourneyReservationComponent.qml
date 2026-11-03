// SPDX-FileCopyrightText: 2026 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

ItineraryReservationComponent {
    id: root

    required property string name
    required property string type
    required property string startTime
    required property string departureTime
    required property string departureDay
    required property string departureLocation
    required property string departureAddress
    required property string departurePlatform
    required property string departureTerminal
    required property string arrivalTime
    required property string arrivalDay
    required property string arrivalLocation
    required property string arrivalAddress
    required property string arrivalPlatform
    required property string arrivalTerminal
    required property string coach
    required property string seat
    required property string iconSource

    headerItem: ReservationHeader {
        Layout.fillWidth: true
        title: root.name
        subtitle: root.departureDay
        iconSource: root.iconSource
    }

    component Stop: RowLayout {
        id: stop

        required property string label
        required property string time
        required property string day
        required property string location
        required property string address
        required property string platform
        required property string terminal

        spacing: Kirigami.Units.largeSpacing

        ColumnLayout {
            Layout.alignment: Qt.AlignTop
            Layout.preferredWidth: Kirigami.Units.gridUnit * 5
            Layout.minimumWidth: Kirigami.Units.gridUnit * 5
            Layout.maximumWidth: Kirigami.Units.gridUnit * 5
            Layout.fillWidth: false
            spacing: Kirigami.Units.smallSpacing

            QQC2.Label {
                Layout.fillWidth: true
                text: stop.label
                color: Kirigami.Theme.textColor
                opacity: 0.7
                wrapMode: Text.WordWrap
            }
            Kirigami.Heading {
                Layout.fillWidth: true
                visible: stop.time.length > 0
                level: 2
                font.weight: Font.DemiBold
                text: stop.time
                wrapMode: Text.WordWrap
            }
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing

            QQC2.Label {
                Layout.fillWidth: true
                visible: stop.day.length > 0
                text: stop.day
                color: Kirigami.Theme.textColor
                opacity: 0.7
                wrapMode: Text.WordWrap
            }
            QQC2.Label {
                Layout.fillWidth: true
                text: stop.location
                textFormat: Text.PlainText
                font.bold: true
                wrapMode: Text.WordWrap
            }
            QQC2.Label {
                Layout.fillWidth: true
                visible: stop.address.length > 0 && stop.address !== stop.location
                text: stop.address
                textFormat: Text.PlainText
                color: Kirigami.Theme.textColor
                opacity: 0.7
                wrapMode: Text.WordWrap
            }
            QQC2.Label {
                Layout.fillWidth: true
                visible: stop.platform.length > 0 || stop.terminal.length > 0
                text: stop.platform.length > 0 ? i18nc("@label", "Platform %1", stop.platform)
                    : i18nc("@label", "Terminal %1", stop.terminal.replace(/^Terminal\s+/i, ""))
                textFormat: Text.PlainText
                wrapMode: Text.WordWrap
            }
        }
    }

    contentItem: ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        Stop {
            Layout.fillWidth: true
            label: i18nc("@label", "Departure")
            time: root.departureTime
            day: ""
            location: root.departureLocation
            address: root.departureAddress
            platform: root.departurePlatform
            terminal: root.departureTerminal
        }
        Kirigami.Separator {
            Layout.fillWidth: true
        }
        Stop {
            Layout.fillWidth: true
            label: i18nc("@label", "Arrival")
            time: root.arrivalTime
            day: root.arrivalDay !== root.departureDay ? root.arrivalDay : ""
            location: root.arrivalLocation
            address: root.arrivalAddress
            platform: root.arrivalPlatform
            terminal: root.arrivalTerminal
        }
        QQC2.Label {
            Layout.fillWidth: true
            visible: root.type === "FlightReservation" && root.startTime.length > 0
            text: i18nc("@label", "Boarding: %1", root.startTime)
            wrapMode: Text.WordWrap
        }
        Flow {
            Layout.fillWidth: true
            visible: root.coach.length > 0 || root.seat.length > 0
            spacing: Kirigami.Units.largeSpacing

            QQC2.Label {
                visible: root.coach.length > 0
                text: i18nc("@label", "Coach %1", root.coach)
            }
            QQC2.Label {
                visible: root.seat.length > 0
                text: i18nc("@label", "Seat %1", root.seat)
            }
        }
    }
}

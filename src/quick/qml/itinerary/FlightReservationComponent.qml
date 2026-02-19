// SPDX-FileCopyrightText: 2024 James Graham <james.h.graham@protonmail.com>
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2

import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

/**
 * @brief A component for a flight itinerary reservation.
 */
ItineraryReservationComponent {
    id: root

    /**
     * @brief The name of the reservation.
     */
    required property string name

    /**
     * @brief The boarding time of the flight.
     *
     * Includes date.
     */
    required property string startTime

    /**
     * @brief The departure time of the flight.
     */
    required property string departureTime

    /**
     * @brief The departure time of the flight.
     */
    required property string departureDay

    /**
     * @brief The departure airport of the flight.
     */
    required property string departureLocation

    /**
     * @brief The address of the departure airport of the flight.
     */
    required property string departureAddress

    /**
     * @brief The arrival airport of the flight.
     */
    required property string arrivalLocation

    /**
     * @brief The address of the arrival airport of the flight.
     */
    required property string arrivalAddress

    headerItem: [
        TransportIcon {
            source: "qrc:/qt/qml/org/kde/pim/mimetreeparser/flight.svg"
            isMask: true
            size: Kirigami.Units.iconSizes.smallMedium
        },
        QQC2.Label {
            text: root.name
            elide: Text.ElideRight

            Layout.fillWidth: true
            Accessible.ignored: true
        },
        QQC2.Label {
            text: root.departureDay
        },
        QQC2.ToolButton {
            display: QQC2.Button.IconOnly
            text: i18nc("@action", "Open Menu")
            icon.name: 'overflow-menu-symbolic'
            onClicked: root.importTrip()

            QQC2.ToolTip.text: text
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
            QQC2.ToolTip.visible: hovered
        }
    ]

    contentItem: ColumnLayout {
        id: topLayout
        spacing: Kirigami.Units.smallSpacing

        QQC2.Label {
            Layout.fillWidth: true
            text: i18nc("flight departure, %1 is airport, %2 is time", "Departure from %1",
                root.departureLocation)
            color: Kirigami.Theme.textColor
            wrapMode: Text.WordWrap
        }
        QQC2.Label {
            Layout.fillWidth: true
            visible: text !== ""
            text: root.departureAddress
        }

        Kirigami.Separator {
            Layout.fillWidth: true
        }

        QQC2.Label {
            Layout.fillWidth: true
            text: i18nc("flight arrival, %1 is airport, %2 is time", "Arrival at %1",
                root.arrivalLocation)
            color: Kirigami.Theme.textColor
            wrapMode: Text.WordWrap
        }
        QQC2.Label {
            Layout.fillWidth: true
            visible: text !== ""
            text: root.arrivalAddress
        }
    }
}

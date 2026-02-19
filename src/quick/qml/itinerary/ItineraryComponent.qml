// SPDX-FileCopyrightText: 2024 Tobias Fella <tobias.fella@kde.org>
// SPDX-FileCopyrightText: 2024 James Graham <james.h.graham@protonmail.com>
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQml.Models
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
import org.kde.pim.mimetreeparser

/**
 * @brief A component to show a preview of a file that can integrate with KDE itinerary.
 */
ColumnLayout {
    id: root

    /**
     * @brief A model with the itinerary preview of the file.
     */
    readonly property ItineraryModel itineraryModel: ItineraryModel {}

    visible: itinerary.count > 0

    Flickable {
        implicitHeight: visible ? Kirigami.Units.gridUnit * 10 : 0

        contentWidth: itineraryRow.implicitWidth + Kirigami.Units.gridUnit
        contentHeight: itineraryRow.implicitHeight
        flickableDirection: Flickable.HorizontalFlick

        Layout.fillWidth: true

        RowLayout {
            id: itineraryRow

            x: 0
            spacing: Kirigami.Units.largeSpacing

            Repeater {
                id: itinerary

                model: root.itineraryModel
                delegate: DelegateChooser {
                    role: "type"
                    DelegateChoice {
                        roleValue: "TrainReservation"
                        delegate: TrainReservationComponent {
                            totalCount: itinerary.count
                            onImportTrip: tripMenu.popup()
                        }
                    }
                    DelegateChoice {
                        roleValue: "LodgingReservation"
                        delegate: HotelReservationComponent {
                            totalCount: itinerary.count
                            onImportTrip: tripMenu.popup()
                        }
                    }
                    DelegateChoice {
                        roleValue: "FoodEstablishmentReservation"
                        delegate: FoodReservationComponent {
                            totalCount: itinerary.count
                            onImportTrip: tripMenu.popup()
                        }
                    }
                    DelegateChoice {
                        roleValue: "FlightReservation"
                        delegate: FlightReservationComponent {
                            totalCount: itinerary.count
                            onImportTrip: tripMenu.popup()
                        }
                    }
                }
            }
        }
    }

    TripMenu {
        id: tripMenu
        parent: root
        itineraryModel: root.itineraryModel
    }
}

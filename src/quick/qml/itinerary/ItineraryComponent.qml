// SPDX-FileCopyrightText: 2024 Tobias Fella <tobias.fella@kde.org>
// SPDX-FileCopyrightText: 2024 James Graham <james.h.graham@protonmail.com>
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

import QtQuick
import QtQuick.Layouts
import QtQml.Models
import org.kde.kirigami as Kirigami
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
    spacing: Kirigami.Units.largeSpacing

    Repeater {
        id: itinerary

        model: root.itineraryModel
        delegate: DelegateChooser {
            role: "type"
            DelegateChoice {
                roleValue: "TrainReservation"
                delegate: JourneyReservationComponent {
                    totalCount: itinerary.count
                    importMenu: tripMenu
                    iconSource: "qrc:/qt/qml/org/kde/pim/mimetreeparser/train.svg"
                }
            }
            DelegateChoice {
                roleValue: "LodgingReservation"
                delegate: HotelReservationComponent {
                    totalCount: itinerary.count
                    importMenu: tripMenu
                }
            }
            DelegateChoice {
                roleValue: "FoodEstablishmentReservation"
                delegate: FoodReservationComponent {
                    totalCount: itinerary.count
                    importMenu: tripMenu
                }
            }
            DelegateChoice {
                roleValue: "BusReservation"
                delegate: JourneyReservationComponent {
                    totalCount: itinerary.count
                    importMenu: tripMenu
                    iconSource: "qrc:/qt/qml/org/kde/pim/mimetreeparser/bus.svg"
                }
            }
            DelegateChoice {
                roleValue: "BoatReservation"
                delegate: JourneyReservationComponent {
                    totalCount: itinerary.count
                    importMenu: tripMenu
                    iconSource: "qrc:/qt/qml/org/kde/pim/mimetreeparser/ferry.svg"
                }
            }
            DelegateChoice {
                roleValue: "EventReservation"
                delegate: EventReservationComponent {
                    totalCount: itinerary.count
                    importMenu: tripMenu
                }
            }
            DelegateChoice {
                roleValue: "FlightReservation"
                delegate: JourneyReservationComponent {
                    totalCount: itinerary.count
                    importMenu: tripMenu
                    iconSource: "qrc:/qt/qml/org/kde/pim/mimetreeparser/flight.svg"
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

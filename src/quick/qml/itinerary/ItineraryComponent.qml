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

    spacing: Kirigami.Units.largeSpacing
    visible: itinerary.count > 0

    Layout.topMargin: visible ? Kirigami.Units.largeSpacing : 0

    Repeater {
        id: itinerary
        model: root.itineraryModel
        delegate: DelegateChooser {
            role: "type"
            DelegateChoice {
                roleValue: "TrainReservation"
                delegate: TrainReservationComponent {}
            }
            DelegateChoice {
                roleValue: "LodgingReservation"
                delegate: HotelReservationComponent {}
            }
            DelegateChoice {
                roleValue: "FoodEstablishmentReservation"
                delegate: FoodReservationComponent {}
            }
            DelegateChoice {
                roleValue: "FlightReservation"
                delegate: FlightReservationComponent {}
            }
        }
    }

    FormCard.FormCard {
        visible: itinerary.count > 0

        FormCard.FormButtonDelegate {
            icon.name: "map-globe"
            text: i18nc("@action", "Send to KDE Itinerary")
            onClicked: root.itineraryModel.sendToItinerary()
        }
    }
}

// SPDX-FileCopyrightText: 2026 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.components as Components
import org.kde.pim.mimetreeparser

Components.ConvergentContextMenu {
    id: root

    /**
     * @brief A model with the itinerary preview of the file.
     */
    required property ItineraryModel itineraryModel

    Kirigami.Action {
        text: i18nc("@action", "Send to KDE Itinerary")
        icon.name: 'org.kde.itinerary'
        onTriggered: root.itineraryModel.sendToItinerary()
    }

    readonly property KDEConnectHandler connectHandler: KDEConnectHandler {}

    readonly property Instantiator connectInstantiator: Instantiator {
        model: root.connectHandler.devices

        delegate: Kirigami.Action {
            required property var modelData

            Component.onCompleted: root.actions.push(this)

            text: i18nc("@action", "Send to %1", modelData.name)
            icon.name: 'kdeconnect'
            onTriggered: root.connectHandler.sendToDevice(root.itineraryModel.path, modelData.deviceId)
        }
    }
}



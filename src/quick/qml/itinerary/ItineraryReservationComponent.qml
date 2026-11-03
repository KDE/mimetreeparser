// SPDX-FileCopyrightText: 2024 James Graham <james.h.graham@protonmail.com>
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2

import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

/**
 * @brief A base component for an itinerary reservation.
 */
FormCard.FormCard {
    id: root

    required property int index
    required property int totalCount
    required property TripMenu importMenu

    /**
     * @brief An item with the header content.
     */
    property alias headerItem: headerContent.children

    /**
     * @brief An item with the main body content.
     */
    property alias contentItem: content.contentItem

    Layout.alignment: Qt.AlignTop
    Layout.fillWidth: true
    Layout.topMargin: index === 0 ? Kirigami.Units.largeSpacing : 0
    Layout.bottomMargin: index === totalCount - 1 ? Kirigami.Units.largeSpacing : 0

    FormCard.AbstractFormDelegate {
        id: headerDelegate

        contentItem: RowLayout {
            spacing: Kirigami.Units.largeSpacing

            RowLayout {
                id: headerContent

                Layout.fillWidth: true
            }

            QQC2.Button {
                id: importButton

                Layout.alignment: Qt.AlignTop
                text: i18nc("@action:button", "Import…")
                icon.name: "document-import"
                down: importButton.pressed || (root.importMenu.opened && root.importMenu.triggerItem === importButton)
                onClicked: root.importMenu.popupBelow(importButton)
            }
        }
        background: null
        bottomPadding: 0
    }

    FormCard.AbstractFormDelegate {
        id: content

        Layout.fillWidth: true
        background: null
    }
}


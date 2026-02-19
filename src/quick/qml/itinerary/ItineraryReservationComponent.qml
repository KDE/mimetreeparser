// SPDX-FileCopyrightText: 2024 James Graham <james.h.graham@protonmail.com>
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

import QtQuick
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

/**
 * @brief A base component for an itinerary reservation.
 */
FormCard.FormCard {
    id: root

    signal importTrip();

    required property int index
    required property int totalCount

    /**
     * @brief An item with the header content.
     */
    property alias headerItem: headerDelegate.contentItem.children

    /**
     * @brief An item with the main body content.
     */
    property alias contentItem: content.contentItem

    Layout.fillHeight: true
    Layout.topMargin: Kirigami.Units.largeSpacing
    Layout.leftMargin: index === 0 ? Kirigami.Units.largeSpacing : 0
    Layout.rightMargin: index === totalCount - 1 ? Kirigami.Units.largeSpacing : 0

    implicitWidth: Math.max(headerDelegate.implicitWidth, content.implicitWidth)

    Component.onCompleted: children[0].radius = Kirigami.Units.cornerRadius

    FormCard.AbstractFormDelegate {
        id: headerDelegate

        contentItem: RowLayout {}
        background: null
        bottomPadding: 0
    }

    FormCard.AbstractFormDelegate {
        id: content

        Layout.fillWidth: true
        background: null
    }
}


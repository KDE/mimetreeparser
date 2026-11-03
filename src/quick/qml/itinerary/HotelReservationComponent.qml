// SPDX-FileCopyrightText: 2024 James Graham <james.h.graham@protonmail.com>
// SPDX-FileCopyrightText: 2026 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2

import org.kde.kirigami as Kirigami

ItineraryReservationComponent {
    id: root

    required property string name
    required property string startTime
    required property string endTime
    required property string address
    required property string location
    required property int nights
    required property url mapUrl

    headerItem: ReservationHeader {
        Layout.fillWidth: true
        title: root.name
        subtitle: root.nights > 0
            ? (root.location.length > 0 ? i18nc("location and duration of a stay", "%1 · %2", root.location, i18np("%1 night", "%1 nights", root.nights))
                                        : i18np("%1 night", "%1 nights", root.nights))
            : root.location
        iconSource: "go-home-symbolic"
    }

    contentItem: ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: Kirigami.Units.largeSpacing
            rowSpacing: Kirigami.Units.smallSpacing

            QQC2.Label {
                Layout.alignment: Qt.AlignTop
                visible: root.startTime.length > 0
                text: i18nc("@label", "Check-in")
                color: Kirigami.Theme.textColor
                opacity: 0.7
            }
            QQC2.Label {
                Layout.fillWidth: true
                visible: root.startTime.length > 0
                text: root.startTime
                wrapMode: Text.WordWrap
            }
            QQC2.Label {
                Layout.alignment: Qt.AlignTop
                visible: root.endTime.length > 0
                text: i18nc("@label", "Check-out")
                color: Kirigami.Theme.textColor
                opacity: 0.7
            }
            QQC2.Label {
                Layout.fillWidth: true
                visible: root.endTime.length > 0
                text: root.endTime
                wrapMode: Text.WordWrap
            }
        }
        RowLayout {
            Layout.fillWidth: true
            visible: root.address.length > 0 || root.mapUrl.toString().length > 0
            spacing: Kirigami.Units.largeSpacing

            QQC2.Label {
                Layout.fillWidth: true
                visible: root.address.length > 0
                text: root.address
                textFormat: Text.PlainText
                color: Kirigami.Theme.textColor
                opacity: 0.7
                wrapMode: Text.WordWrap
            }
            QQC2.Button {
                Layout.alignment: Qt.AlignTop | Qt.AlignRight
                visible: root.mapUrl.toString().length > 0
                text: i18nc("@action:button", "Open Map")
                icon.name: "map-symbolic"
                onClicked: Qt.openUrlExternally(root.mapUrl)
            }
        }
    }
}

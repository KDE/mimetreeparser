// SPDX-FileCopyrightText: 2026 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

RowLayout {
    id: root

    required property string title
    required property string iconSource
    property string subtitle

    spacing: Kirigami.Units.largeSpacing

    TransportIcon {
        Layout.alignment: Qt.AlignTop
        source: root.iconSource
        isMask: true
        size: Kirigami.Units.iconSizes.medium
        Accessible.ignored: true
    }
    ColumnLayout {
        Layout.fillWidth: true
        spacing: Kirigami.Units.smallSpacing

        Kirigami.Heading {
            Layout.fillWidth: true
            level: 2
            font.weight: Font.DemiBold
            text: root.title
            textFormat: Text.PlainText
            wrapMode: Text.WordWrap
        }
        QQC2.Label {
            Layout.fillWidth: true
            visible: root.subtitle.length > 0
            text: root.subtitle
            textFormat: Text.PlainText
            color: Kirigami.Theme.textColor
                opacity: 0.7
            wrapMode: Text.WordWrap
        }
    }
}

// SPDX-FileCopyrightText: 2016 Michael Bohlender <michael.bohlender@kdemail.net>
// SPDX-FileCopyrightText: 2022 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls as QQC2

import org.kde.pim.mimetreeparser
import org.kde.kirigami as Kirigami

Item {
    id: root

    property string content
    property bool embedded: true
    property bool autoLoadImages: false

    property string searchString
    property int contentHeight: textEdit.height

    onSearchStringChanged: {
        //This is a workaround because otherwise the view will not take the ViewHighlighter changes into account.
        textEdit.text = root.content
    }

    QQC2.TextArea {
        id: textEdit
        objectName: "textView"
        background: null
        readOnly: true
        textFormat: TextEdit.RichText
        wrapMode: TextEdit.Wrap
        padding: 0

        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
        }

        text: content.substring(0, 100000).replace(/\u00A0/g,' ') //The TextEdit deals poorly with messages that are too large.
        color: embedded ? Kirigami.Theme.disabledTextColor : Kirigami.Theme.textColor
        onLinkActivated: link => Qt.openUrlExternally(link)

        onHoveredLinkChanged: if (hoveredLink.length > 0) {
            applicationWindow().hoverLinkIndicator.text = hoveredLink;
        } else {
            applicationWindow().hoverLinkIndicator.text = "";
        }

        //Kube.ViewHighlighter {
        //    textDocument: textEdit.textDocument
        //    searchString: root.searchString
        //}
    }
}

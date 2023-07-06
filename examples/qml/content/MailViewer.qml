// SPDX-FileCopyrightText: 2022 Carl Schwan <carl@carlschwan.eu>
// SPDX-FileCopyrightText: 2022 Devin Lin <espidev@gmail.com>
// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import QtGraphicalEffects 1.15

import org.kde.mimetreeparser 1.0
import org.kde.kirigami 2.20 as Kirigami
import org.kde.kitemmodels 1.0 as KItemModels

Kirigami.Page {
    id: root

    property alias message: mailPartView.message

    readonly property string subject: mailPartView.subject
    readonly property string from: mailPartView.from
    readonly property string sender: mailPartView.sender
    readonly property string to: mailPartView.to
    readonly property date dateTime: mailPartView.dateTime

    Kirigami.Theme.colorSet: Kirigami.Theme.View
    Kirigami.Theme.inherit: false

    padding: Kirigami.Units.largeSpacing * 2

    title: i18n("Message viewer")

    header: QQC2.ToolBar {
        id: mailHeader

        padding: root.padding

        Kirigami.Theme.inherit: false
        Kirigami.Theme.colorSet: Kirigami.Theme.View

        background: Item {
            Rectangle {
                anchors.fill: parent
                color: Kirigami.Theme.alternateBackgroundColor
            }

            Kirigami.Separator {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
            }

            Kirigami.Separator {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
            }
        }

        GridLayout {
            width: mailHeader.width - mailHeader.leftPadding - mailHeader.rightPadding
            rowSpacing: Kirigami.Units.smallSpacing
            columnSpacing: Kirigami.Units.smallSpacing

            columns: 3

            QQC2.Label {
                text: i18n('Subject:')
                font.bold: true

                Layout.rightMargin: Kirigami.Units.largeSpacing
            }

            QQC2.Label {
                text: root.subject
                elide: Text.ElideRight

                Layout.fillWidth: true
            }

            QQC2.Label {
                text: root.dateTime.toLocaleString(Qt.locale(), Locale.ShortFormat)
                horizontalAlignment: Text.AlignRight
            }

            QQC2.Label {
                text: i18n('From:')
                font.bold: true

                Layout.rightMargin: Kirigami.Units.largeSpacing
            }

            QQC2.Label {
                text: root.from
                elide: Text.ElideRight

                Layout.fillWidth: true
                Layout.columnSpan: 2
            }

            QQC2.Label {
                text: i18n('Sender:')
                font.bold: true
                visible: root.sender.length > 0 && root.sender !== root.from

                Layout.rightMargin: Kirigami.Units.largeSpacing
            }

            QQC2.Label {
                visible: root.sender.length > 0 && root.sender !== root.from
                text: root.sender
                elide: Text.ElideRight

                Layout.fillWidth: true
                Layout.columnSpan: 2
            }

            QQC2.Label {
                text: i18n('To:')
                font.bold: true

                Layout.rightMargin: Kirigami.Units.largeSpacing
            }

            QQC2.Label {
                text: root.to
                elide: Text.ElideRight

                Layout.fillWidth: true
                Layout.columnSpan: 2
            }
        }
    }

    MailPartView {
        id: mailPartView
        anchors.fill: parent
    }

    footer: QQC2.ToolBar {
        padding: root.padding

        Kirigami.Theme.inherit: false
        Kirigami.Theme.colorSet: Kirigami.Theme.View

        background: Item {
            Kirigami.Separator {
                anchors {
                    left: parent.left
                    right: parent.right
                    top: undefined
                    bottom:  parent.bottom
                }
            }
        }

        Flow {
            anchors.fill: parent
            spacing: Kirigami.Units.smallSpacing
            Repeater {
                model: mailPartView.attachmentModel

                delegate: AttachmentDelegate {
                    name: model.name
                    type: model.type
                    icon.name: model.iconName

                    clip: true

                    actionIcon: 'download'
                    actionTooltip: i18n("Save attachment")
                    onExecute: mailPartView.attachmentModel.saveAttachmentToDisk(mailPartView.attachmentModel.index(index, 0))
                    onClicked: mailPartView.attachmentModel.openAttachment(mailPartView.attachmentModel.index(index, 0))
                    onPublicKeyImport: mailPartView.attachmentModel.importPublicKey(mailPartView.attachmentModel.index(index, 0))
                }
            }
        }
    }
}

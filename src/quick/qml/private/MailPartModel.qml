// SPDX-FileCopyrightText: 2016 Michael Bohlender <michael.bohlender@kdemail.net>
// SPDX-FileCopyrightText: 2022 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQml.Models
import org.kde.pim.mimetreeparser
import org.kde.kirigami as Kirigami

DelegateModel {
    id: root

    property string searchString: ""
    property bool autoLoadImages: false
    property int padding: Kirigami.Units.largeSpacing
    property url icalCustomComponent
    required property var urlHandler

    delegate: RowLayout {
        id: partDelegate

        required property int type
        required property string content
        required property bool isEmbedded
        required property int sidebarSecurityLevel

        required property var encryptionInfo
        required property var signatureInfo
        required property var errorInfo

        readonly property bool isEncrypted: encryptionInfo.securityLevel !== PartModel.Unknow
        readonly property bool isSigned: signatureInfo.securityLevel !== PartModel.Unknow

        width: ListView.view.width
        spacing: Kirigami.Units.smallSpacing

        function getType(securityLevel: int): int {
            if (securityLevel === PartModel.Good) {
                return Kirigami.MessageType.Positive
            }
            if (securityLevel === PartModel.Bad) {
                return Kirigami.MessageType.Error
            }
            if (securityLevel === PartModel.NotSoGood) {
                return Kirigami.MessageType.Warning
            }
            return Kirigami.MessageType.Information
        }

        function getColor(securityLevel: int): color {
            if (securityLevel === PartModel.Good) {
                return Kirigami.Theme.positiveTextColor
            }
            if (securityLevel === PartModel.Bad) {
                return Kirigami.Theme.negativeTextColor
            }
            if (securityLevel === PartModel.NotSoGood) {
                return Kirigami.Theme.neutralTextColor
            }
            return "transparent"
        }

        QQC2.Control {
            id: sidebar

            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.preferredWidth: Kirigami.Units.smallSpacing
            Layout.fillHeight: true

            visible: partDelegate.sidebarSecurityLevel !== PartModel.Unknown

            background: Rectangle {
                id: border
                color: getColor(partDelegate.sidebarSecurityLevel)
            }
        }

        ColumnLayout {
            spacing: Kirigami.Units.smallSpacing

            Layout.leftMargin: Kirigami.Units.gridUnit - (sidebar.visible ? Kirigami.Units.largeSpacing : 0) - Kirigami.Units.largeSpacing
            Layout.rightMargin: Kirigami.Units.gridUnit

            Banner {
                property var info: partDelegate.encryptionInfo
                visible: partDelegate.isEncrypted
                iconName: info.iconName
                type: getType(info.securityLevel)
                text: info.summary + (info.details.length ? (' ' + info.details.join(' ')) : '');
                onLinkActivated: (link) => root.urlHandler.handleClick(link, QQC2.ApplicationWindow.window)

                Layout.fillWidth: true
            }

            Banner {
                property var info: partDelegate.signatureInfo
                visible: partDelegate.isSigned
                iconName: info.iconName
                type: getType(info.securityLevel)
                text: info.summary + (info.details.length ? (' ' + info.details.join(' ')) : '');
                onLinkActivated: (link) => root.urlHandler.handleClick(link, QQC2.ApplicationWindow.window)

                Layout.fillWidth: true
            }

            Loader {
                id: partLoader

                Layout.preferredHeight: item ? item.contentHeight : 0
                Layout.fillWidth: true

                Component.onCompleted: {
                    switch (partDelegate.type) {
                        case PartModel.Plain:
                            partLoader.setSource("TextPart.qml", {
                                content: partDelegate.content,
                                embedded: partDelegate.isEmbedded,
                            })
                            break
                        case PartModel.Html:
                            partLoader.setSource("HtmlPart.qml", {
                                content: partDelegate.content,
                            })
                            break;
                        case PartModel.Error:
                            partLoader.setSource("ErrorPart.qml", {
                                errorString: partDelegate.errorInfo.summary + partDelegate.errorInfo.details.length ? (' ' + partDelegate.errorInfo.details.join(' ')) : '',
                            })
                            break;
                        case PartModel.Encapsulated:
                            partLoader.setSource("MailPart.qml", {
                                rootIndex: root.modelIndex(index),
                                model: root.model,
                                sender: model.sender,
                                date: model.date,
                            })
                            break;
                        case PartModel.Ical:
                            partLoader.setSource(root.icalCustomComponent ? root.icalCustomComponent : "ICalPart.qml", {
                                content: partDelegate.content,
                            })
                            break;
                    }
                }

                Binding {
                    target: partLoader.item
                    property: "searchString"
                    value: root.searchString
                    when: partLoader.status === Loader.Ready
                }
                Binding {
                    target: partLoader.item
                    property: "autoLoadImages"
                    value: root.autoLoadImages
                    when: partLoader.status === Loader.Ready
                }
            }
        }
    }
}

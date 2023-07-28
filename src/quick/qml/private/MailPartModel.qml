// SPDX-FileCopyrightText: 2016 Michael Bohlender <michael.bohlender@kdemail.net>
// SPDX-FileCopyrightText: 2022 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.15
import QtQml.Models 2.2
import org.kde.pim.mimetreeparser 1.0
import org.kde.kirigami 2.19 as Kirigami

DelegateModel {
    id: root

    property string searchString: ""
    property bool autoLoadImages: false
    property int padding: Kirigami.Units.largeSpacing

    delegate: RowLayout {
        id: partColumn

        width: ListView.view.width

        function getType(securityLevel) {
            if (securityLevel == "good") {
                return Kirigami.MessageType.Positive
            }
            if (securityLevel == "bad") {
                return Kirigami.MessageType.Error
            }
            if (securityLevel == "notsogood") {
                return Kirigami.MessageType.Warning
            }
            return Kirigami.MessageType.Information
        }

        function getColor(securityLevel) {
            if (securityLevel == "good") {
                return Kirigami.Theme.positiveTextColor
            }
            if (securityLevel == "bad") {
                return Kirigami.Theme.negativeTextColor
            }
            if (securityLevel == "notsogood") {
                return Kirigami.Theme.neutralTextColor
            }
            return "transparent"
        }

        function getDetails(signatureDetails) {
            let details = "";
            if (signatureDetails.keyMissing) {
                details += i18n("This message has been signed using the key %1.", signatureDetails.keyId) + "\n";
                details += i18n("The key details are not available.")
            } else {
                details += i18n("This message has been signed using the key %1 by %2.", signatureDetails.keyId, signatureDetails.signer) + "\n";
                if (signatureDetails.keyRevoked) {
                    details += "\n" + i18n("The key was revoked.")
                }
                if (signatureDetails.keyExpired) {
                    details += "\n" + i18n("The key has expired.")
                }
                if (signatureDetails.keyIsTrusted) {
                    details += "\n" + i18n("You are trusting this key.")
                }
                if (!signatureDetails.signatureIsGood && !signatureDetails.keyRevoked && !signatureDetails.keyExpired && !signatureDetails.keyIsTrusted) {
                    details += "\n" + i18n("The signature is invalid.")
                }
            }
            return details
        }

        QQC2.Control {
            Layout.preferredWidth: Kirigami.Units.smallSpacing
            Layout.fillHeight: true

            visible: model.encrypted

            background: Rectangle {
                id: border

                color: getColor(model.securityLevel)
                opacity: 0.5
            }

            QQC2.ToolTip.text: getDetails(model.encryptionSecurityLevel)
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        }

        ColumnLayout {
            Banner {
                iconName: "mail-encrypted"
                type: getType(model.encryptionSecurityLevel)
                visible: model.encrypted
                text: model.encryptionDetails.keyId.length === 0 ? i18n("This message is encrypted but we don't have the key for it.") : i18n("This message is encrypted to the key: %1", model.encryptionDetails.keyId);

                Layout.fillWidth: true
            }

            Banner {
                iconName: 'mail-signed'
                visible: model.signed
                type: getType(model.signatureSecurityLevel)
                text: getDetails(model.signatureDetails)

                Layout.fillWidth: true
            }

            Loader {
                id: partLoader

                Layout.preferredHeight: item ? item.contentHeight : 0
                Layout.fillWidth: true
                Layout.leftMargin: root.padding
                Layout.rightMargin: root.padding

                Component.onCompleted: {
                    switch (model.type + 0) {
                        case PartModel.Plain:
                            partLoader.setSource("TextPart.qml", {
                                content: model.content,
                                embedded: model.embedded,
                            })
                            break
                        case PartModel.Html:
                            partLoader.setSource("HtmlPart.qml", {
                                content: model.content,
                            })
                            break;
                        case PartModel.Error:
                            partLoader.setSource("ErrorPart.qml", {
                                errorType: model.errorType,
                                errorString: model.errorString,
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
                            partLoader.setSource("ICalPart.qml", {
                                content: model.content,
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

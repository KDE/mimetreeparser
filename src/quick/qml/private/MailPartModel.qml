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
    property url icalCustomComponent

    delegate: RowLayout {
        id: partColumn

        width: ListView.view.width - Kirigami.Units.largeSpacing
        x: Kirigami.Units.smallSpacing

        function getType(securityLevel) {
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

        function getColor(securityLevel) {
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

        function getDetails(signatureDetails) {
            let details = "";
            if (signatureDetails.keyMissing) {
                details += i18ndc("mimetreeparser", "@info", "This message was signed with certificate %1.", signatureDetails.keyId) + "\n";
                details += i18ndc("mimetreeparser", "@info", "The certificate details are not available.")
            } else {
                details += i18ndc("mimetreeparser", "@info", "This message was signed by %1 with certificate %2.", signatureDetails.signer, signatureDetails.keyId) + "\n";
                if (signatureDetails.keyRevoked) {
                    details += "\n" + i18ndc("mimetreeparser", "@info", "The certificate was revoked.")
                }
                if (signatureDetails.keyExpired) {
                    details += "\n" + i18ndc("mimetreeparser", "@info", "The certificate has expired.")
                }
                if (signatureDetails.keyIsTrusted) {
                    details += "\n" + i18ndc("mimetreeparser", "@info", "The certificate is certified.")
                }
                if (!signatureDetails.signatureIsGood && !signatureDetails.keyRevoked && !signatureDetails.keyExpired && !signatureDetails.keyIsTrusted) {
                    details += "\n" + i18ndc("mimetreeparser", "@info", "The signature is invalid.")
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
                text: !model.encryptionDetails.keyId ? i18n("This message is encrypted but you don't have a matching secret key.") : i18n("This message is encrypted for: %1", model.encryptionDetails.keyId);

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
                            partLoader.setSource(root.icalCustomComponent ? root.icalCustomComponent : "ICalPart.qml", {
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

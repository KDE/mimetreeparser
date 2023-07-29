// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
// SPDX-FileCopyrightText: 2016 Michael Bohlender <michael.bohlender@kdemail.net>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.14 as Kirigami
import QtQuick.Controls 2.15 as Controls
import org.kde.pim.mimetreeparser 1.0
import org.kde.kitemmodels 1.0 as KItemModels

ListView {
    id: root

    property alias message: messageParser.message
    readonly property string subject: messageParser.subject
    readonly property string from: messageParser.from
    readonly property string sender: messageParser.sender
    readonly property string to: messageParser.to
    readonly property date dateTime: messageParser.date

    property alias rootIndex: visualModel.rootIndex
    property alias padding: visualModel.padding
    property alias searchString: visualModel.searchString
    property alias autoLoadImages: visualModel.autoLoadImages
    property var attachmentModel: messageParser.attachments

    topMargin: padding
    bottomMargin: padding

    spacing: Kirigami.Units.smallSpacing

    model: MailPartModel {
        id: visualModel
        model: messageParser.parts
    }

    MessageParser {
        id: messageParser
    }
}

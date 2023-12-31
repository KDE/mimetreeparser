// SPDX-FileCopyrightText: 2023 Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs 6.2
import org.kde.kirigami 2.20 as Kirigami
import org.kde.pim.mimetreeparser 1.0
import org.kde.mimetreeparser 1.0

Kirigami.ApplicationWindow {
    id: root

    readonly property Kirigami.Action openFileAction: Kirigami.Action {
        text: i18n("Open File")
        shortcut: "Ctrl+O"
        onTriggered: fileDialog.open()
    }

    FileDialog {
        id: fileDialog
        title: i18n("Choose file")
        onAccepted: messageHandler.open(selectedFile)
    }

    MessageHandler {
        id: messageHandler
        objectName: "MessageHandler"
        onMessageOpened: message => {
            pageStack.currentItem.message = message
        }
    }

    pageStack.initialPage: MailViewer {
    }
}

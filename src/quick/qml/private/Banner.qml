// SPDX-FileCopyrightText: 2023 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Templates 2.15 as T
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.20 as Kirigami

/**
 * @brief An banner Item with support for informational, positive,
 * warning and error types, and with support for associated actions.
 *
 * Banner can be used to inform or interact with the user
 * without requiring the use of a dialog and are positionned in the footer
 * or header of a page. For inline content, see org::kde::kirigami::InlineMessage.
 *
 * The Banner is hidden by default.
 *
 * Optionally, actions can be added which are shown alongside an
 * optional close button on the right side of the Item. If more
 * actions are set than can fit, an overflow menu is provided.
 *
 * Example usage:
 * @code{.qml}
 * Banner {
 *     type: Kirigami.MessageType.Error
 *
 *     text: "My error message"
 *
 *     actions: [
 *         Kirigami.Action {
 *             icon.name: "edit"
 *             text: "Action text"
 *             onTriggered: {
 *                 // do stuff
 *             }
 *         },
 *         Kirigami.Action {
 *             icon.name: "edit"
 *             text: "Action text"
 *             onTriggered: {
 *                 // do stuff
 *             }
 *         }
 *     ]
 * }
 * @endcode
 */
T.ToolBar {
    id: root

    /**
     * @brief This signal is emitted when a link is hovered in the message text.
     * @param The hovered link.
     */
    signal linkHovered(string link)

    /**
     * @brief This signal is emitted when a link is clicked or tapped in the message text.
     * @param The clicked or tapped link.
     */
    signal linkActivated(string link)

    /**
     * @brief This property holds the link embedded in the message text that the user is hovering over.
     */
    readonly property alias hoveredLink: label.hoveredLink

    /**
     * @brief This property holds the message type.
     *
     * The following values are allowed:
     * * ``Kirigami.MessageType.Information``
     * * ``Kirigami.MessageType.Positive``
     * * ``Kirigami.MessageType.Warning``
     * * ``Kirigami.MessageType.Error``
     *
     * default: ``Kirigami.MessageType.Information``
     *
     * @property Kirigami.MessageType type
     */
    property int type: Kirigami.MessageType.Information

    /**
     * @brief This property holds the message title.
     */
    property string title

    /**
     * @brief This property holds the message text.
     */
    property string text

    /**
     * @brief This property holds the icon name (default to predefined icons
     * depending on the title if not set).
     */
    property string iconName

    /**
     * @brief This property holds whether the close button is displayed.
     *
     * default: ``false``
     */
    property bool showCloseButton: false

    /**
     * This property holds the list of Kirigami Actions to show in the banner's
     * internal kirigami::ActionToolBar.
     *
     * Actions are added from left to right. If more actions
     * are set than can fit, an overflow menu is provided.
     */
    property list<Kirigami.Action> actions

    padding: Kirigami.Units.smallSpacing

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, contentHeight + topPadding + bottomPadding)

    visible: false

    contentItem: GridLayout {
        columns: 3
        columnSpacing: Kirigami.Units.mediumSpacing

        Item {
            Layout.preferredWidth: closeButton.visible ? closeButton.implicitWidth : Kirigami.Units.iconSizes.medium
            Layout.preferredHeight: closeButton.visible ? closeButton.implicitWidth : Kirigami.Units.iconSizes.medium
            Layout.alignment: Qt.AlignTop
            Layout.rowSpan: 2

            Kirigami.Icon {
                source: {
                    if (root.iconName.length > 0) {
                        return root.iconName
                    }
                    switch (root.type) {
                    case Kirigami.MessageType.Positive:
                        return "emblem-positive";
                    case Kirigami.MessageType.Warning:
                        return "data-warning";
                    case Kirigami.MessageType.Error:
                        return "data-error";
                    default:
                        return "data-information";
                    }
                }

                anchors.centerIn: parent
            }
        }

        Kirigami.Heading {
            id: heading

            level: 2
            text: root.title
            visible: text.length > 0

            Layout.row: visible ? 0 : 1
            Layout.column: 1
        }

        Kirigami.SelectableLabel {
            id: label

            color: Kirigami.Theme.textColor
            wrapMode: Text.WordWrap

            text: root.text

            onLinkHovered: link => root.linkHovered(link)
            onLinkActivated: link => root.linkActivated(link)

            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop

            Layout.row: heading.visible ? 1 : 0
            Layout.column: 1
        }

        QQC2.ToolButton {
            id: closeButton

            visible: root.showCloseButton
            text: i18ndc("mimetreeparser", "@action:button", "Close")

            icon.name: "dialog-close"
            display: QQC2.ToolButton.IconOnly

            onClicked: root.visible = false

            Layout.alignment: Qt.AlignTop

            QQC2.ToolTip.visible: Kirigami.Settings.tabletMode ? closeButton.pressed : closeButton.hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
            QQC2.ToolTip.text: text

            Layout.rowSpan: 2
            Layout.column: 2
        }

        Kirigami.ActionToolBar {
            id: actionsLayout

            flat: false
            actions: root.actions
            visible: root.actions.length > 0
            alignment: Qt.AlignRight

            Layout.column: 0
            Layout.columnSpan: 3
            Layout.row: 2
        }
    }

    background: Item {
        id: backgroundItem

        readonly property color color: switch (root.type) {
        case Kirigami.MessageType.Positive:
            return Kirigami.Theme.positiveTextColor;
        case Kirigami.MessageType.Warning:
            return Kirigami.Theme.neutralTextColor;
        case Kirigami.MessageType.Error:
            return Kirigami.Theme.negativeTextColor;
        default:
            return Kirigami.Theme.activeTextColor;
        }

        Rectangle {
            opacity: 0.2
            color: backgroundItem.color
            radius: Kirigami.Units.smallSpacing
            anchors.fill: parent
        }
    }
}

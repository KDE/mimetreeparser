// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "mimetreeparser_widgets_export.h"

#include <KMime/Message>
#include <QMainWindow>

#include <memory>

class QToolBar;

namespace MimeTreeParser
{
namespace Widgets
{

/*!
 * \class MimeTreeParser::MessageViewerDialog
 * \inmodule MimeTreeParserWidgets
 * \inheaderfile MimeTreeParserWidgets/MessageViewerDialog
 *
 * \author Carl Schwan <carl.schwan@gnupg.com>
 * \brief MessageViewerWindow that displays the given email stored in the file.
 */
class MIMETREEPARSER_WIDGETS_EXPORT MessageViewerWindow : public QMainWindow
{
    Q_OBJECT
    Q_PROPERTY(QList<std::shared_ptr<KMime::Message>> messages READ messages WRITE setMessages NOTIFY messagesChanged)

public:
    /*!
     * \brief Constructs a MessageViewerWindow
     * \param parent The parent widget
     */
    explicit MessageViewerWindow(QWidget *parent = nullptr);
    /*!
     * \brief Destroys the MessageViewerWindow
     */
    ~MessageViewerWindow() override;

    /*!
     * \brief Returns the tool bar of the viewer window
     * \return A pointer to the toolbar
     */
    [[nodiscard]] QToolBar *toolBar() const;

    /*!
     * \brief Returns the list of messages being displayed
     * \return A list of messages
     */
    [[nodiscard]] QList<std::shared_ptr<KMime::Message>> messages() const;
    /*!
     * \brief Sets the messages to display
     * \param messages The list of messages to display
     */
    void setMessages(const QList<std::shared_ptr<KMime::Message>> &messages);

Q_SIGNALS:
    /*!
     * \brief Emitted when the displayed messages change
     */
    void messagesChanged();

private:
    MIMETREEPARSER_WIDGETS_NO_EXPORT void initGUI();

    class Private;
    std::unique_ptr<Private> const d;
};

} // end namespace Widgets
} // end namespace MimeTreeParser

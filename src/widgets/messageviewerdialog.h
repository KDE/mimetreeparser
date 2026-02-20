// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "mimetreeparser_widgets_export.h"

#include <KMime/Message>
#include <QDialog>

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
 * \brief MessageViewerDialog that displays the given email stored in the file.
 */
class MIMETREEPARSER_WIDGETS_EXPORT MessageViewerDialog : public QDialog
{
    Q_OBJECT

public:
    /*!
     * \brief Constructs a MessageViewerDialog from a list of messages
     * \param messages The messages to display
     * \param parent The parent widget
     */
    explicit MessageViewerDialog(const QList<std::shared_ptr<KMime::Message>> &messages, QWidget *parent = nullptr);
    /*!
     * \brief Constructs a MessageViewerDialog from a file
     * \param fileName The path to the file containing messages
     * \param parent The parent widget
     */
    explicit MessageViewerDialog(const QString &fileName, QWidget *parent = nullptr);
    /*!
     * \brief Destroys the MessageViewerDialog
     */
    ~MessageViewerDialog() override;

    /*!
     * \brief Returns the tool bar of the dialog
     * \return A pointer to the toolbar
     */
    [[nodiscard]] QToolBar *toolBar() const;

    /*!
     * \brief Returns the list of messages being displayed
     * \return A list of messages
     */
    [[nodiscard]] QList<std::shared_ptr<KMime::Message>> messages() const;

private:
    MIMETREEPARSER_WIDGETS_NO_EXPORT void initGUI();

    class Private;
    std::unique_ptr<Private> const d;
};

} // end namespace Widgets
} // end namespace MimeTreeParser

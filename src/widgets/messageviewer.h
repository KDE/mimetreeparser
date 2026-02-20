// SPDX-FileCopyrightText: 2023 Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include "mimetreeparser_widgets_export.h"

#include <KMime/Message>

#include <QSplitter>
#include <QWidget>

#include <memory>

class QPainter;

namespace MimeTreeParser
{
namespace Widgets
{
/*!
 * \class MimeTreeParser::MessageViewer
 * \inmodule MimeTreeParserWidgets
 * \inheaderfile MimeTreeParserWidgets/MessageViewer
 *
 * \author Carl Schwan <carl.schwan@gnupg.com>
 * \brief MessageViewer that displays the given std::shared_ptr<KMime::Message>
 */
class MIMETREEPARSER_WIDGETS_EXPORT MessageViewer : public QSplitter
{
public:
    /*!
     * \brief Constructs a MessageViewer widget
     * \param parent The parent widget
     */
    explicit MessageViewer(QWidget *parent = nullptr);
    /*!
     * \brief Destroys the MessageViewer
     */
    ~MessageViewer() override;

    /*!
     * \brief Returns the currently displayed message
     * \return A shared pointer to the message
     */
    [[nodiscard]] std::shared_ptr<KMime::Message> message() const;
    /*!
     * \brief Sets the message to display
     * \param message The message to display
     */
    void setMessage(const std::shared_ptr<KMime::Message> message);

    /*!
     * \brief Returns whether a fixed width font is used
     * \return True if fixed font is enabled
     */
    [[nodiscard]] bool fixedFont() const;
    /*!
     * \brief Sets whether to use a fixed width font
     * \param checked True to enable fixed width font
     */
    void setFixedFont(bool checked);

    /*!
     * \brief Returns the subject of the current message
     * \return The message subject
     */
    [[nodiscard]] QString subject() const;

    /*!
     * \brief Prints the message to the given painter
     * \param painter The painter to draw on
     * \param width The width available for printing
     */
    void print(QPainter *painter, int width);

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // end namespace Widgets
} // end namespace MimeTreeParser
